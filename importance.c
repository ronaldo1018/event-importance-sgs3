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
#include "importance.h"
#include "config.h"
#include "netlink.h"
#include "activity.h"
#include "touch.h"
#include "core.h"
#include "thread.h"
#include "timer.h"
#include "vector.h"
#include "parse.h"
#include "common.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h> // exit()
#include <unistd.h> // getpid()
#include <errno.h>
#include <signal.h> // signal(), siginterrupt()
#include <sys/resource.h> // setpriority()
#include <sys/socket.h>

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
vector *pidListVec[CONFIG_NUM_OF_CORE];

/**
 * @brief data structure to store thread information
 */
THREADATTR threadSet[PID_MAX+1];

/**
 * @brief data structure to store cpu core information
 */
COREATTR coreSet[CONFIG_NUM_OF_CORE];

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

/**
 * @brief current cpu frequency
 */
int curFreq;

/**
 * @brief maximum cpu frequency available
 */
int maxFreq = 0;

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
	if(DEBUG_INFO || DEBUG_DVFS_INFO)
	{
		initialize_debug();
	}
	initialize_data_structures(); // must be initialize first because other initialize function might use
	prioritize_self(); // change nice of this process to highest
	initialize_cores(); // must precede initialize_threads() because we need data structure in initialize_cores() to count utilization
	initialize_touch();
	initialize_threads(); // must precede initailize_activity() becaise we need data structure in initialize_threads()
	initialize_activity();
	initialize_timer();

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

	if(handle_proc_event() == -1) // listen to events and react
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
	INFO(("change importance %d to %d\n", pid, importance));
	int coreId = threadSet[pid].coreId;
	if(firstAssign)
		coreSet[coreId].sumOfImportance += threadSet[pid].importance;
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
	for(i = 0; i < CONFIG_NUM_OF_CORE; i++)
	{
		vector_dispose(pidListVec[i]);
		free(pidListVec[i]);
	}
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
	int ret;
	INFO(("prioritize self\n"));

	ret = setpriority(PRIO_PROCESS, getpid(), -20); // let this method to have the highest priority
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

	while(!needExit)
	{
		INFO(("wait for message\n"));
		ret = recv(nlSock, &cnprocMsg, sizeof(cnprocMsg), 0);

		if(ret == 0)
		{
			/* shutdown? */
			return 0;
		}
		else if(ret == -1)
		{
			if(errno == EINTR) continue;
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
	for(i = 0; i < CONFIG_NUM_OF_CORE; i++)
	{
		pidListVec[i] = (vector *)malloc(sizeof(vector));
		vector_init(pidListVec[i], sizeof(int), 0, NULL);
	}

	INFO(("initialize THREADATTR\n"));
	memset(threadSet, 0, sizeof(THREADATTR) * PID_MAX);

	INFO(("initialize COREATTR\n"));
	memset(coreSet, 0, sizeof(COREATTR) * CONFIG_NUM_OF_CORE);
}

/**
 * @brief produce_importance produce thread importance based on event happened
 *
 * @param cnprocMsg
 */
static void produce_importance(NLCN_CNPROC_MSG *cnprocMsg)
{
	int i;
	int appearedThrPid, disappearedThrPid, activityChangeThrPid;
	int parentThrPid;
	int length;
	int screen_on;
	int timer_no;
	static int check_activity_counter = 0;
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
		case SCREEN_EVENT:
			INFO(("[screen]\n"));
			screen_on = cnprocMsg->proc_ev.event_data.screen.screen_on;
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
		case TIMER_TICK_EVENT:
			INFO(("[timer]\n"));
			// TODO: need to check threads are aging or in touch event, must discard timer change
			timer_no = cnprocMsg->proc_ev.event_data.timer.timer_no;
			switch(timer_no)
			{
				case 0: // utilization sampling
					INFO(("timer: timer0 tick\n"));

					// check if activity change
					check_activity_counter++;
					if(check_activity_counter >= CONFIG_CHECK_ACTIVITY_TIME)
					{
						vector_remove_all(becomeBGThrVec);
						vector_remove_all(becomeFGThrVec);
						if(check_activity_change(becomeBGThrVec, becomeFGThrVec))
						{
							INFO(("activity: bg/fg change\n"));
							length = vector_length(becomeBGThrVec);
							for(i = 0; i < length; i++)
							{
								activityChangeThrPid = ((int *)becomeBGThrVec->elems)[i];
								change_importance(activityChangeThrPid, IMPORTANCE_LOW, false);
							}
							
							length = vector_length(becomeFGThrVec);
							for(i = 0; i < length; i++)
							{
								activityChangeThrPid = ((int *)becomeFGThrVec->elems)[i];
								change_importance(activityChangeThrPid, IMPORTANCE_MID, false);
							}
						}
						check_activity_counter = 0;
					}

					// check if screen touch if use polling
					if(CONFIG_POLLING_TOUCH_STATUS)
					{
						if(check_screen_touch() && !touchIsOn)
						{
							INFO(("screen: touch\n"));
							touchIsOn = true;
							importance_mid_to_high();
						}
					}

					// aging
					if(CONFIG_TURN_ON_AGING)
					{
						if(agingIsOn)
							aging();
					}
					break;
				case 1: // temp high
					INFO(("timer: timer1 tick\n"));
					importance_back_to_mid();
					reenable_touch();
					touchIsOn = false;
					break;
				default:
					ERR(("no such timer_no %d\n", timer_no));
					break;
			}
			break;
		case TOUCH_EVENT:
			INFO(("[touch]\n"));
			touchIsOn = true;
			curFreq = maxFreq;
			setFreq(maxFreq); // early set frequency to highest
			importance_mid_to_high();
			break;
		default:
			INFO(("unknown event!\n"));
			break;
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
	int timer_no;
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
		case SCREEN_EVENT:
			if(vector_length(importanceChangeThrVec) != 0) // some threads' importance has changed
			{
				DPM();
				migration();
				DVFS();
				prioritize(importanceChangeThrVec);
			}
			break;
		case TIMER_TICK_EVENT:
			timer_no = cnprocMsg->proc_ev.event_data.timer.timer_no;
			switch(timer_no)
			{
				case 0: // utilization sampling
					if(!touchIsOn)
					{
						calculate_utilization();
						DPM();
						migration();
						DVFS();
					}
					turn_on_sampling_timer();
					break;
				case 1: // temp high
					if(vector_length(importanceChangeThrVec) != 0)
					{
						prioritize(importanceChangeThrVec);
						DVFS();
					}
					break;
				default:
					ERR(("no such timer_no %d\n", timer_no));
					break;
			}
			break;
		case TOUCH_EVENT:
			if(vector_length(importanceChangeThrVec) != 0)
			{
				prioritize(importanceChangeThrVec);
				//DVFS();
			}
			break;
		default:
			INFO(("unknown event!\n"));
			break;
	}
}

/**
 * @brief importance_mid_to_high let foreground threads becomes high importance after user makes a touch
 */
static void importance_mid_to_high(void)
{
	int i, length;
	int pid;

	INFO(("fg thr mid to high\n"));
	getCurActivityThrs(curActivityThrVec);
	if(vector_length(curActivityThrVec) > 0)
	{
		length = vector_length(curActivityThrVec);
		for(i = 0; i < length; i++)
		{
			pid = ((int *)curActivityThrVec->elems)[i];
			change_importance(pid, IMPORTANCE_HIGH, false);
		}
		turn_on_temp_high_timer();
	}
	else
	{
		ERR(("cannot get current foreground activity threads\n"));
		destruction();
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief importance_back_to_mid let foreground threads from high back to mid importance after enough time 
 */
static void importance_back_to_mid(void)
{
	INFO(("fg thr back to mid\n"));
	cur_activity_importance_to_mid();
}

/**
 * @brief cur_activity_importance_to_low when foreground threads becomes background threads or screen is off, change foreground thread to low importance
 */
static void cur_activity_importance_to_low(void)
{
	int pid;
	int i, length;

	INFO(("fg thr to low\n"));
	getCurActivityThrs(curActivityThrVec);
	length = vector_length(curActivityThrVec);
	for(i = 0; i < length; i++)
	{
		pid = ((int *)curActivityThrVec->elems)[i];
		change_importance(pid, IMPORTANCE_LOW, false);
	}
}

/**
 * @brief cur_activity_importance_to_mid when background threads become foreground threads or screen turns on, change new foreground threads to mid importance
 */
static void cur_activity_importance_to_mid(void)
{
	int pid;
	int i, length;

	INFO(("fg thr to mid\n"));
	getCurActivityThrs(curActivityThrVec);
	length = vector_length(curActivityThrVec);
	for(i = 0; i < length; i++)
	{
		pid = ((int *)curActivityThrVec->elems)[i];
		INFO(("vector_get %d\n", pid));
		change_importance(pid, IMPORTANCE_MID, false);
	}
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

