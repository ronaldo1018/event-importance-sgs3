/* Copyright (C) 
 * 2013 - Po-Hsien Tseng <steve13814@gmail.com>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */
/**
 * @file importance.c
 * @brief control thread importance
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#include "ImportanceClient.h"
#include "importance.h"
#include "config.h"
#include "netlink.h"
#include "touch.h"
#include "core.h"
#include "thread.h"
#include "timer.h"
#include "vector.h"
#include "parse.h"
#include "common.h"
#include "debug.h"
#include "my_sysinfo.h"
#include "device_events.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <stdio.h>
#include <stdlib.h> // exit()
#include <unistd.h> // getpid()
#include <errno.h>
#include <signal.h> // signal(), siginterrupt()
#include <string.h>
#include <limits.h>
#include <sys/resource.h> // setpriority()
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/system_properties.h> // __system_property_get
#pragma GCC diagnostic pop

// global variables
/**
 * @brief file description id that opens netlink connection
 */
int nlSock = -1;

/**
 * @brief whether aging mechanism is on
 */
bool agingIsOn = false;

/**
 * @brief whether user just make a screen touch
 */
bool touchIsOn = false;

/**
 * @brief store thread id which temporarily becomes mid
 */
vector *appearedThrVec;

/**
 * @brief store thread id which just become background thread
 */
vector *becomeBGThrVec;

/**
 * @brief store thread id which just become foreground thread
 */
vector *becomeFGThrVec;

/**
 * @brief store foreground activity threads
 */
vector *curActivityThrVec;

/**
 * @brief store threads that importance are just changed
 */
vector *importanceChangeThrVec;

/**
 * @brief store pid list in each core
 */
vector **pidListVec;

/**
 * @brief data structure to store thread information
 */
THREADATTR threadSet[PID_MAX+1];

/**
 * @brief data structure to store cpu core information
 */
COREATTR *coreSet;

/**
 * @brief minimal utilization core id
 */
int minUtilCoreId = 0;

/**
 * @brief maximum utilization core id
 */
int maxUtilCoreId = 0;

/**
 * @brief minimal importance core id
 */
int minImpCoreId = 0;

/**
 * @brief maximum importance core id
 */
int maxImpCoreId = 0;

/**
 * @brief the last pid that is aging
 */
int lastAgingPid = 1;

/**
 * @brief elapse time since last utilization calculation
 */
float elapseTime;

int epollfd;
extern int timerfd[N_TIMERS];

// variables or functions only used in this file
/**
 * @brief flag used to notify whether need to exit
 */
static volatile bool needExit = false;

static void prioritize_self(void);
static void initialize_data_structures(void);
static void on_sigint(int unused);
static int handle_proc_event();
static void produce_importance(NLCN_CNPROC_MSG *cnprocMsg);
static void do_action(NLCN_CNPROC_MSG *cnprocMsg);
static void importance_mid_to_high(void);
static void importance_back_to_mid(void);
static void cur_activity_importance_to_low(void);
static void cur_activity_importance_to_mid(void);
static void aging(void);
static void run_on_foreground_threads(void (*fn)(pid_t));
static int event_loop();
static void wait_for_boot_completed();

/**
 * @brief main entry function
 *
 * @param argc
 * @param argv
 *
 * @return process exit status
 */
int main(int argc, char **argv)
{
    wait_for_boot_completed();

	if(DEBUG_INFO || DEBUG_DVFS_INFO)
	{
		initialize_debug();
	}
	initialize_data_structures(); // must be initialize first because other initialize function might use
	prioritize_self(); // change nice of this process to highest
	initialize_cores(); // must precede initialize_threads() because we need data structure in initialize_cores() to count utilization

    // Need to connect before initialize touch as touch uses shared memory
    if (connect_service() != 0)
    {
        exit(EXIT_FAILURE);
    }

	initialize_touch();
	initialize_threads(); // must precede initailize_activity() becaise we need data structure in initialize_threads()
	initialize_timer();

    turn_on_timer(TIMER_UTILIZATION_SAMPLING);

	signal(SIGINT, &on_sigint); // catch ctrl+c before stop
	siginterrupt(SIGINT, true);

	if((nlSock = get_netlink_socket()) == -1) // retrieve a netlink socket
	{
		exit(EXIT_FAILURE);
	}

	if(set_proc_event_listen(nlSock, true) == -1) // subscribe proc events
	{
		close(nlSock);
		exit(EXIT_FAILURE);
	}

	if(event_loop() == -1) // listen to events and react
	{
		destruction();
		exit(EXIT_FAILURE);
	}

	destruction();
	return 0;
}

/**
 * @brief change importance of a thread, update thread and core information
 *
 * @param pid
 * @param importance
 * @param firstAssign if thread is newly appeared, need to assign firstAssign to true
 */
void change_importance(int pid, enum IMPORTANCE_VALUE importance, bool firstAssign)
{
    if (threadSet[pid].importance == importance)
    {
        return;
    }
	INFO(("change importance %d to %d\n", pid, importance));
	int coreId = threadSet[pid].coreId;
	if(firstAssign)
		coreSet[coreId].sumOfImportance += importance;
	else
		coreSet[coreId].sumOfImportance += importance - threadSet[pid].importance;
	threadSet[pid].importance = importance;
	vector_push(importanceChangeThrVec, &pid);
}

/**
 * @brief destruction destroy vector used and unsubscribe netlink proc events
 */
void destruction(void)
{
	int i;
	INFO(("destruction\n"));

	vector_dispose(appearedThrVec);
	vector_dispose(becomeBGThrVec);
	vector_dispose(becomeFGThrVec);
	vector_dispose(curActivityThrVec);
	vector_dispose(importanceChangeThrVec);
	free(appearedThrVec);
	free(becomeBGThrVec);
	free(becomeFGThrVec);
	free(curActivityThrVec);
	free(importanceChangeThrVec);

	for(i = 0; i < my_get_nprocs_conf(); i++)
	{
		vector_dispose(pidListVec[i]);
		free(pidListVec[i]);
	}
    free(pidListVec);
    free(coreSet);
	destroy_timer();
	if(nlSock != -1)
	{
		set_proc_event_listen(nlSock, false); // unsubscribe proc events
		close(nlSock);
	}
}

/**
 * @brief prioritize_self change nice of this process to highest
 */
static void prioritize_self(void)
{
	int ret = -1;
	INFO(("prioritize self\n"));

	// ret = setpriority(PRIO_PROCESS, getpid(), -20); // let this method to have the highest priority
    ret = 0;
	INFO(("pid = %d, ret = %d\n", getpid(), ret));
	if(ret)
	{
		ERR(("cannot set nice value, ret: %d\n", ret));
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief on_sigint modify exit flag to true
 *
 * @param unused
 */
static void on_sigint(int unused)
{
	INFO(("caught sigint\n"));
	needExit = true;
}

/**
 * @brief handle_proc_event listen to netlink events and do corresponding actions
 *
 * @param nlSock netlink socket file descriptor
 *
 * @return 0 on success, -1 otherwise
 */
static int handle_proc_event()
{
	int ret;
	NLCN_CNPROC_MSG cnprocMsg;
	INFO(("handle proc events\n"));

    INFO(("wait for message\n"));
    ret = recv(nlSock, &cnprocMsg, sizeof(cnprocMsg), 0);

    if(ret == 0)
    {
        /* shutdown? */
        return 0;
    }
    else if(ret == -1)
    {
        ERR(("netlink recv error\n"));
        return -1;
    }

    switch(cnprocMsg.proc_ev.what)
    {
        case PROC_EVENT_NONE:
        case PROC_EVENT_EXEC:
        case PROC_EVENT_UID:
        case PROC_EVENT_GID:
        case PROC_EVENT_SID:
            break; // ignore these events
        default:
            vector_remove_all(importanceChangeThrVec);
            produce_importance(&cnprocMsg); // modify event affected threads
            do_action(&cnprocMsg); // do the 6 component
            break;
    }
	return 0;
}

/**
 * @brief initialize_vectors initialize vectors for later use
 */
static void initialize_data_structures(void)
{
	int i;
	INFO(("initialize vectors\n"));
	appearedThrVec = (vector *)malloc(sizeof(vector));
	becomeBGThrVec = (vector *)malloc(sizeof(vector));
	becomeFGThrVec = (vector *)malloc(sizeof(vector));
	curActivityThrVec = (vector *)malloc(sizeof(vector));
	importanceChangeThrVec = (vector *)malloc(sizeof(vector));
	vector_init(appearedThrVec, sizeof(int), 0, NULL);
	vector_init(becomeBGThrVec, sizeof(int), 0, NULL);
	vector_init(becomeFGThrVec, sizeof(int), 0, NULL);
	vector_init(curActivityThrVec, sizeof(int), 0, NULL);
	vector_init(importanceChangeThrVec, sizeof(int), 0, NULL);
    pidListVec = (vector**) malloc (sizeof (vector) * (unsigned int) my_get_nprocs_conf());
	for(i = 0; i < my_get_nprocs_conf(); i++)
	{
		pidListVec[i] = (vector *)malloc(sizeof(vector));
		vector_init(pidListVec[i], sizeof(int), 0, NULL);
	}

	INFO(("initialize THREADATTR\n"));
	memset(threadSet, 0, sizeof(THREADATTR) * PID_MAX);

	INFO(("initialize COREATTR\n"));
    coreSet = (COREATTR*) calloc ((unsigned int) my_get_nprocs_conf(), sizeof (COREATTR));

    INFO(("initialize epollfd\n"));
    epollfd = epoll_create(16);

#if CONFIG_USE_FIFO
    INFO(("initialize devices events FIFO\n"));
    initialize_fifo();
#endif
}

/**
 * @brief produce_importance produce thread importance based on event happened
 *
 * @param cnprocMsg
 */
static void produce_importance(NLCN_CNPROC_MSG *cnprocMsg)
{
	int appearedThrPid, disappearedThrPid;
	int parentThrPid;
	INFO(("produce importance\n"));

	switch(cnprocMsg->proc_ev.what)
	{
		case PROC_EVENT_FORK:
			INFO(("[fork]\n"));
			appearedThrPid = cnprocMsg->proc_ev.event_data.fork.child_pid;
			parentThrPid = cnprocMsg->proc_ev.event_data.fork.parent_pid;
			initialize_thread(appearedThrPid); // initialize thread must be called here instead of later because we will modify importance
			// inherit parent thread's importance
			INFO(("appeared pid %d inherits parent %d's imp %d\n", appearedThrPid, parentThrPid, threadSet[parentThrPid].importance));
			change_importance(appearedThrPid, threadSet[parentThrPid].importance, true);
			break;
		case PROC_EVENT_EXIT:
			INFO(("[exit]\n"));
			disappearedThrPid = cnprocMsg->proc_ev.event_data.exit.process_pid;
			INFO(("disappeared pid %d imp %d exit\n", disappearedThrPid, threadSet[disappearedThrPid].importance));
			destroy_thread(disappearedThrPid);
			break;
		default:
			INFO(("unknown event!\n"));
			break;
	}
}

void handle_screen_onoff(int screen_on)
{
    INFO(("[screen]\n"));

    // produce importance
    switch(screen_on)
    {
        case 0: // screen off
            INFO(("screen turn off\n"));
            if(CONFIG_TURN_ON_AGING)
                agingIsOn = false;
            cur_activity_importance_to_low();
            break;
        case 1: // screen on
            INFO(("screen turn on\n"));
            if(CONFIG_TURN_ON_AGING)
            {
                agingIsOn = true;
            }
            cur_activity_importance_to_mid();
            break;
        default:
            ERR(("no such screen event\n"));
            break;
    }

    // do action
    if(vector_length(importanceChangeThrVec) != 0) // some threads' importance has changed
    {
        DPM();
        migration();
        DVFS();
        prioritize(importanceChangeThrVec);
    }
}

void handle_touch()
{
    // produce importance
    INFO(("[touch]\n"));
    touchIsOn = true;
    set_curFreq(get_maxFreq());
    setFreq(get_maxFreq()); // early set frequency to highest
    importance_mid_to_high();
    turn_on_timer(TIMER_TMP_HIGH);

    // do_action
    if(vector_length(importanceChangeThrVec) != 0)
    {
        prioritize(importanceChangeThrVec);
        //DVFS();
        // commented out in "refine DVFS policy"
    }
}

/**
 * @brief do_action do actions based on event happened
 *
 * @param cnprocMsg
 */
static void do_action(NLCN_CNPROC_MSG *cnprocMsg)
{
	int appearedThrPid;
	INFO(("do action\n"));

	switch(cnprocMsg->proc_ev.what)
	{
		case PROC_EVENT_FORK:
			appearedThrPid = cnprocMsg->proc_ev.event_data.fork.child_pid;
			allocation(appearedThrPid);
			prioritize(importanceChangeThrVec);
			break;
		case PROC_EVENT_EXIT:
			break;
		default:
			INFO(("unknown event!\n"));
			break;
	}
}

static void importance_mid_to_high_callback(pid_t thread_id)
{
    change_importance(thread_id, IMPORTANCE_HIGH, false);
}

/**
 * @brief importance_mid_to_high let foreground threads becomes high importance after user makes a touch
 */
static void importance_mid_to_high(void)
{
	INFO(("fg thr mid to high\n"));

    run_on_foreground_threads(importance_mid_to_high_callback);
}

/**
 * @brief importance_back_to_mid let foreground threads from high back to mid importance after enough time 
 */
static void importance_back_to_mid(void)
{
	INFO(("fg thr back to mid\n"));
	cur_activity_importance_to_mid();
}

static void cur_activity_importance_to_low_callback(pid_t thread_id)
{
    change_importance(thread_id, IMPORTANCE_LOW, false);
}

/**
 * @brief cur_activity_importance_to_low when foreground threads becomes background threads or screen is off, change foreground thread to low importance
 */
static void cur_activity_importance_to_low(void)
{
	INFO(("fg thr to low\n"));
    run_on_foreground_threads(cur_activity_importance_to_low_callback);
}

static void cur_activity_importance_to_mid_callback(pid_t thread_id)
{
    INFO(("vector_get %d\n", thread_id));
    change_importance(thread_id, IMPORTANCE_MID, false);
}

/**
 * @brief cur_activity_importance_to_mid when background threads become foreground threads or screen turns on, change new foreground threads to mid importance
 */
static void cur_activity_importance_to_mid(void)
{
	INFO(("fg thr to mid\n"));
    run_on_foreground_threads(cur_activity_importance_to_mid_callback);
}

/**
 * @brief aging take turns raising each background thread to mid
 */
static void aging(void)
{
	// TODO: modify aging time
	INFO(("aging\n"));
	int pidIterator = lastAgingPid + 1;
	if(pidIterator > PID_MAX)
		pidIterator = 1;
	while(pidIterator != lastAgingPid)
	{
		if(threadSet[pidIterator].isUsed && threadSet[pidIterator].importance == IMPORTANCE_LOW && threadSet[pidIterator].util != 0)
		{
			INFO(("aging old %d new %d\n", lastAgingPid, pidIterator));
			if(!threadSet[lastAgingPid].isSysThr)
				change_importance(lastAgingPid, IMPORTANCE_LOW, false);
			change_importance(pidIterator, IMPORTANCE_MID, false);
			lastAgingPid = pidIterator;
			break;
		}
		pidIterator++;
		if(pidIterator > PID_MAX)
			pidIterator = 1;
	}
	if(pidIterator == lastAgingPid)
	{
		INFO(("no low thread, get period back to 1s\n"));
		lastAgingPid = 1;
	}
}

static void run_on_foreground_threads(void (*fn)(pid_t))
{
    FILE *f;
    pid_t thread_id;

    f = fopen("/dev/cpuctl/apps/tasks", "r");
    while (fscanf(f, "%d", &thread_id) != EOF)
    {
        fn(thread_id);
    }
    fclose(f);
}

static void on_timer_utilization_sampling()
{
    INFO(("timer: timer0 tick\n"));

    // aging
    if(CONFIG_TURN_ON_AGING)
    {
        if(agingIsOn)
            aging();
    }

    if(!touchIsOn)
    {
        calculate_utilization();
        DPM();
        migration();
        DVFS();
    }
}

static void on_timer_tmp_high()
{
    INFO(("timer: timer1 tick\n"));
    importance_back_to_mid();
    reenable_touch();
    touchIsOn = false;

    if(vector_length(importanceChangeThrVec) != 0)
    {
        prioritize(importanceChangeThrVec);
        DVFS();
    }
    turn_off_timer(TIMER_TMP_HIGH);
}

static int event_loop()
{
    struct epoll_event epollev[16];
    int nfds;
    int fd;
    int i;
    uint64_t exp; // expired time of timerfds
    struct FIFO_DATA fifo_data;

    while (!needExit)
    {
        nfds = epoll_wait(epollfd, epollev, 16, -1);
        INFO(("epoll_wait returned %d events\n", nfds));
        for (i = 0; i < nfds; i++)
        {
            fd = epollev[i].data.fd;
            if (fd == nlSock) {
                handle_proc_event();
            } else if (fd == timerfd[TIMER_UTILIZATION_SAMPLING]) {
                // TODO: need to check threads are aging or in touch event, must discard timer change
                read(fd, &exp, sizeof(exp));
                on_timer_utilization_sampling();
            } else if (fd == timerfd[TIMER_TMP_HIGH]) {
                read(fd, &exp, sizeof(exp));
                on_timer_tmp_high();
            } else if (fd == get_fifo_fd()) {
                read(fd, &fifo_data, sizeof (fifo_data));
                vector_remove_all(importanceChangeThrVec);
                handle_device_events(fifo_data);
            } else {
                ERR(("Unknown fd %d from epoll_wait!\n", fd));
                needExit = true;
            }
        }
    }

    return 0;
}

static void wait_for_boot_completed()
{
    char prop[PATH_MAX];
    memset(prop, 0, PATH_MAX);
    while (true)
    {
        if (__system_property_get("sys.boot_completed", prop) > 0)
        {
            if (strcmp (prop, "1") == 0)
            {
                break;
            }
        }
        INFO(("Waiting for sys.boot_completed\n"));
        sleep(1);
        continue;
    }
}

int get_epoll_fd()
{
    return epollfd;
}
