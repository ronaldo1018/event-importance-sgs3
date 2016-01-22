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
 * @file thread.c
 * @brief control threads
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#include "thread.h"
#include "config.h"
#include "core.h"
#include "importance.h"
#include "activity.h"
#include "timer.h"
#include "parse.h"
#include "common.h"
#include "debug.h"
#include "my_sysinfo.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include <stdio.h>
#include <stdlib.h> // atoi()
#include <ctype.h> // isdigit()
#include <string.h>
#include <dirent.h> // opendir(), readdir(), closedir()
#include <sys/time.h> // gettimeofday()
#include <sys/stat.h> // stat()
#include <sys/resource.h> // setpriority()
#pragma GCC diagnostic pop

extern THREADATTR threadSet[];
extern COREATTR *coreSet;
extern int minUtilCoreId;
extern int maxUtilCoreId;
extern int minImpCoreId;
extern int maxImpCoreId;
extern vector *curActivityThrVec;
extern vector **pidListVec;
extern float elapseTime;

// variables or functions only used in this file
/**
 * @brief record last sampling time
 */
static struct timeval oldTime;

/**
 * @brief initialize_threads initialize all threads and change importance
 */
void initialize_threads(void)
{
	DIR *d;
	struct dirent *de;
	FILE *fp;
	char buff[BUFF_SIZE];
	struct timezone timez;
	int pid;
	float execTime;
	INFO(("initialize threads\n"));

	/* find all threads in /proc */
	if((d = opendir("/proc")) != NULL)
	{
		while((de = readdir(d)) != 0)
		{
			if(isdigit(de->d_name[0]))
			{
				pid = atoi(de->d_name);
				initialize_thread(pid);
			}
		}
		closedir(d);
	}
	else
	{
		ERR(("cannot open directory /proc\n"));
	}

	/* use allstat kernel module to get each threads execution time */
	fp = fopen(ALLSTAT_PATH, "r");
	if(fp)
	{
		while(fgets(buff, BUFF_SIZE, fp))
		{
			sscanf(buff, "%d %f", &pid, &execTime);
			threadSet[pid].execTime = execTime;
		}
		fclose(fp);
	}

	/* initialize timestamp, for later calculating elapse time */
	gettimeofday(&oldTime, &timez);
}

/**
 * @brief initialize_thread initialize a thread in threadSet
 *
 * @param pid
 */
void initialize_thread(int pid)
{
	char buff[BUFF_SIZE];
	struct stat statBuf;

	memset(&threadSet[pid], 0, sizeof(THREADATTR));
	
	// get uid, if uid >= 10000 then it's an app
	sprintf(buff, "/proc/%d", pid);
	if(stat(buff, &statBuf))
	{
		INFO(("thread not exist\n"));
		return;
	}
	if(statBuf.st_uid >= 10000) // user thread
	{
		threadSet[pid].isSysThr = false;
		change_importance(pid, IMPORTANCE_LOW, true);
	}
	else // system thread
	{
		threadSet[pid].isSysThr = true;
		change_importance(pid, IMPORTANCE_MID, true); // set to mid because we can get utilization of this thread for dvfs, dpm, ....
	}

	threadSet[pid].isUsed = 1;
	assign_core(pid, 0, true);
}

/**
 * @brief destroy_thread distroy a thread in threadSet, update coreSet information
 *
 * @param pid
 */
void destroy_thread(int pid)
{
	int coreId;
	INFO(("destroy thread %d\n", pid));

	coreId = threadSet[pid].coreId;
	threadSet[pid].isUsed = 0;
	coreSet[coreId].sumOfImportance -= threadSet[pid].importance;
	coreSet[coreId].numOfThreads--;
	if(threadSet[pid].importance >= IMPORTANCE_MID)
		coreSet[coreId].numOfMidThreads--;
}

/**
 * @brief calculate_utilization calculate each thread's new utilization
 */
void calculate_utilization(void)
{
	int i;
    int pid;
	float execTime;
	FILE *fp;
	int coreId;
	char buff[BUFF_SIZE];
	struct timeval t;
	struct timezone timez;
	unsigned long long cpuinfo[10], busy, nice_busy, idle, busysub, nice_busysub, idlesub;
	DVFS_INFO(("calculate utilization\n"));

	// clear coreSet util
	for(i = 0; i < my_get_nprocs_conf(); i++)
	{
		coreSet[i].util = 0;
		coreSet[i].midUtil = 0;
		coreSet[i].numOfRunningThreads = 0;
		coreSet[i].numOfRunningMidThreads = 0;
	}

	// update timestamp
	gettimeofday(&t, &timez);
	elapseTime = t.tv_sec - oldTime.tv_sec + (float)(t.tv_usec - oldTime.tv_usec) / 1000000.0f;
	DVFS_INFO(("elapseTime = %f\n", elapseTime));
	oldTime.tv_sec = t.tv_sec;
	oldTime.tv_usec = t.tv_usec;

	fp = fopen(CPUINFO_PATH, "r");
	if(fp)
	{
		fgets(buff, BUFF_SIZE, fp); // ignore first line
		while(fgets(buff, BUFF_SIZE, fp))
		{
			if(strstr(buff, "cpu"))
			{
				sscanf(buff, "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &coreId, &cpuinfo[0], &cpuinfo[1], &cpuinfo[2], &cpuinfo[3], &cpuinfo[4], &cpuinfo[5], &cpuinfo[6], &cpuinfo[7], &cpuinfo[8], &cpuinfo[9]); // time(unit: jiffies) spent of all cpus for: user nice system idle iowait irq softirq stead guest
				busy = cpuinfo[0] + cpuinfo[1] + cpuinfo[2] + cpuinfo[4] + cpuinfo[5] + cpuinfo[6] + cpuinfo[7] + cpuinfo[8] + cpuinfo[9];
				busysub = busy - coreSet[coreId].busy;
				nice_busy = cpuinfo[1];
				nice_busysub = nice_busy - coreSet[coreId].nice_busy;
				idle = cpuinfo[3];
				idlesub = idle - coreSet[coreId].idle;
				if(busysub + idlesub == 0)
				{
					coreSet[coreId].util = 0;
					coreSet[coreId].midUtil = 0;
				}
				else
				{
					coreSet[coreId].util = (float)(busysub) / (busysub + idlesub) * get_curFreq();
					coreSet[coreId].midUtil = (float)(busysub - nice_busysub) / (busysub + idlesub) * get_curFreq();
				}
				INFO(("core %d: util %f midUtil %f\n", coreId, coreSet[coreId].util, coreSet[coreId].midUtil));
				coreSet[coreId].busy = busy;
				coreSet[coreId].nice_busy = nice_busy;
				coreSet[coreId].idle = idle;
			}
			else // string 'cpu' is not found
			{
				break;
			}
		}
		fclose(fp);
	}

	/* update min/max util/imp core */
	updateMinMaxCore();

	/* use allstat kernel module to update each threads execution time and pid list in each core*/
	for(i = 0; i < my_get_nprocs_conf(); i++)
	{
		vector_remove_all(pidListVec[i]);
	}

	fp = fopen(ALLSTAT_PATH, "r");
	if(fp)
	{
		while(fgets(buff, BUFF_SIZE, fp))
		{
			sscanf(buff, "%d %f", &pid, &execTime);
			vector_push(pidListVec[i], &pid);
			threadSet[pid].util = (execTime - threadSet[pid].execTime) * get_curFreq();
			if(threadSet[pid].util != 0)
			{
				coreSet[threadSet[pid].coreId].numOfRunningThreads++;
				if(threadSet[pid].importance >= IMPORTANCE_MID)
					coreSet[threadSet[pid].coreId].numOfRunningMidThreads++;
			}
		}
		fclose(fp);
	}
}

/**
 * @brief allocation allocate new thread to a core based on its importance
 *
 * @param pid
 */
void allocation(int pid)
{
	if(!CONFIG_TURN_ON_ALLOCATION)
		return;
	INFO(("allocation\n"));

	if(threadSet[pid].importance >= IMPORTANCE_MID)
	{
		assign_core(pid, minImpCoreId, false);
		INFO(("Put pid %u of importance %d to core %d\n", pid, threadSet[pid].importance, minImpCoreId));
	}
	else // importance = low
	{
		assign_core(pid, minUtilCoreId, false);
		INFO(("Put pid %u of importance %d to core %d\n", pid, threadSet[pid].importance, minUtilCoreId));
	}
}

/**
 * @brief prioritize prioritize threads based on their importance
 *
 * @param importanceChangeThrVec
 */
void prioritize(vector *importanceChangeThrVec)
{
	unsigned int i, length;
	int ret, pid;
	if(!CONFIG_TURN_ON_PRIORITIZE)
		return;
	INFO(("prioritize\n"));

	length = vector_length(importanceChangeThrVec);
	for(i = 0; i < length; i++)
	{
		pid = ((int *)importanceChangeThrVec->elems)[i];
		switch(threadSet[pid].importance)
		{
			case IMPORTANCE_LOW:
				ret = setpriority(PRIO_PROCESS, pid, CONFIG_NICE_LOW);
				break;
			case IMPORTANCE_MID:
				ret = setpriority(PRIO_PROCESS, pid, CONFIG_NICE_MID);
				break;
			case IMPORTANCE_HIGH:
				ret = setpriority(PRIO_PROCESS, pid, CONFIG_NICE_HIGH);
				break;
			default:
				ERR(("unknown importance %d\n", threadSet[pid].importance));
				destruction();
				exit(EXIT_FAILURE);
				break;
		}
		if(ret)
			INFO(("thread %d disappeared...\n", pid));
		else
			INFO(("pid = %d, nice = %d\n", pid, threadSet[pid].importance == IMPORTANCE_LOW? CONFIG_NICE_LOW: threadSet[pid].importance == IMPORTANCE_MID? CONFIG_NICE_MID: CONFIG_NICE_HIGH));
	}
}
