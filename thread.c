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
 * @version 20130317
 * @date 2013-03-17
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
#include <stdio.h>
#include <stdlib.h> // atoi()
#include <ctype.h> // isdigit()
#include <string.h>
#include <dirent.h> // opendir(), readdir(), closedir()
#include <sys/time.h> // gettimeofday()
#include <sys/stat.h> // stat()
#include <sys/resource.h> // setpriority()

extern THREADATTR threadSet[];
extern COREATTR coreSet[];
extern vector *curActivityThrVec;
extern int curFreq;

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
	struct timezone timez;
	int pid;
	INFO(("initialize threads\n"));

	memset(threadSet, 0, sizeof(THREADATTR) * PID_MAX);

	if((d = opendir("/proc")) != NULL)
	{
		while((de = readdir(d)) != 0)
		{
			if(isdigit(de->d_name[0]))
			{
				pid = atoi(de->d_name);
				initialize_thread(pid);
				if(threadSet[pid].isSysThr)
					change_importance(pid, IMPORTANCE_MID, true);
				else
					change_importance(pid, IMPORTANCE_LOW, true); // set to mid because we can get utilization of this thread for dvfs, dpm...., but priority of this thread will not be changed
			}
		}
		closedir(d);
	}
	else
	{
		fprintf(stderr, "cannot open directory /proc\n");
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
	bool failbit;

	memset(&threadSet[pid], 0, sizeof(THREADATTR));
	
	// get uid, if uid >= 10000 then it's an app
	sprintf(buff, "/proc/%d", pid);
	if(stat(buff, &statBuf))
	{
		INFO(("thread not exist\n"));
		return;
	}
	if(statBuf.st_uid >= 10000)
		threadSet[pid].isSysThr = false;
	else
		threadSet[pid].isSysThr = true;

	// get original nice value and on which core
	sprintf(buff, "/proc/%d/stat", pid);
	parseInt2(buff, 18, &threadSet[pid].originalNice, 38, &threadSet[pid].coreId, &failbit);
	// get utime and stime
	parseInt2(buff, 13, &threadSet[pid].oldutime, 14, &threadSet[pid].oldstime, &failbit);
	INFO(("initialize thread %d: s:%s n:%d c:%d ut:%d st:%d\n", pid, threadSet[pid].isSysThr?"s":"u", threadSet[pid].originalNice, threadSet[pid].coreId, threadSet[pid].oldutime, threadSet[pid].oldstime));
	if(failbit) return; // thread disappeared..
	threadSet[pid].isUsed = 1;
	assign_core(pid, threadSet[pid].coreId, true);
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
	coreSet[coreId].util -= threadSet[pid].util;
	if(threadSet[pid].importance >= IMPORTANCE_MID)
		coreSet[coreId].midUtil -= threadSet[pid].util;
	coreSet[coreId].sumOfImportance -= threadSet[pid].importance;
	coreSet[coreId].numOfThreads--;
}

/**
 * @brief calculate_utilization calculate each thread's new utilization
 */
void calculate_utilization(void)
{
	int i;
	DIR *d;
	struct dirent *de;
	bool failbit;
	int pid;
	char buff[BUFF_SIZE];
	struct timeval t;
	struct timezone timez;
	float elapseTime;
	float newUtil;
	DVFS_INFO(("calculate utilization\n"));
	
	// clear coreSet util
	for(i = 0; i < NUM_OF_CORE; i++)
	{
		coreSet[i].util = 0;
		coreSet[i].midUtil = 0;
	}

	// update timestamp
	gettimeofday(&t, &timez);
	elapseTime = t.tv_sec - oldTime.tv_sec + (float)(t.tv_usec - oldTime.tv_usec) / 1000000.0;
	DVFS_INFO(("elapseTime = %f\n", elapseTime));
	oldTime.tv_sec = t.tv_sec;
	oldTime.tv_usec = t.tv_usec;
	
	if((d = opendir("/proc")) != NULL)
	{
		while((de = readdir(d)) != 0)
		{
			if(isdigit(de->d_name[0]))
			{
				pid = atoi(de->d_name);
				sprintf(buff, "/proc/%d/stat", pid);
				parseInt2(buff, 13, &threadSet[pid].utime, 14, &threadSet[pid].stime, &failbit);
				if(failbit)
				{
					// thread disappeared
					continue;
				}

				// notice: curFreq may change between two utilization samplings, but we use last value to estimate for simplicity
				newUtil = (float)(threadSet[pid].utime - threadSet[pid].oldutime + threadSet[pid].stime - threadSet[pid].oldstime) * UTILIZATION_PROMOTION_RATIO / CONFIG_HZ / elapseTime * curFreq;
				if(newUtil != 0)
					DVFS_INFO(("pid %d has new util %f\n", pid, newUtil));
				coreSet[threadSet[pid].coreId].util += newUtil;
				if(threadSet[pid].importance >= IMPORTANCE_MID)
					coreSet[threadSet[pid].coreId].midUtil += newUtil;
				threadSet[pid].util = newUtil;
				threadSet[pid].oldutime = threadSet[pid].utime;
				threadSet[pid].oldstime = threadSet[pid].stime;
			}
		}
		closedir(d);
	}
	else
	{
		fprintf(stderr, "cannot open directory /proc\n");
	}
}

/**
 * @brief allocation allocate new thread to a core based on its importance
 *
 * @param pid
 */
void allocation(int pid)
{
	int i, minCoreId;
	if(!CONFIG_TURN_ON_ALLOCATION)
		return;
	INFO(("allocation\n"));

	minCoreId = 0;
	if(threadSet[pid].importance >= IMPORTANCE_MID)
	{
		for(i = 1; i < NUM_OF_CORE; i++)
		{
			if(coreSet[i].sumOfImportance < coreSet[minCoreId].sumOfImportance)
			{
				minCoreId = i;
			}
		}
	}
	else
	{
		for(i = 1; i < NUM_OF_CORE; i++)
		{
			if(coreSet[i].util < coreSet[minCoreId].util)
			{
				minCoreId = i;
			}
		}
	}
	INFO(("Put pid %u of importance %d to core %d\n", pid, threadSet[pid].importance, minCoreId));
	assign_core(pid, minCoreId, false);
}

/**
 * @brief prioritize prioritize threads based on their importance
 *
 * @param importanceChangeThrVec
 */
void prioritize(vector *importanceChangeThrVec)
{
	int i, length, ret;
	int pid;
	if(!CONFIG_TURN_ON_PRIORITIZE)
		return;
	INFO(("prioritize\n"));

	length = vector_length(importanceChangeThrVec);
	for(i = 0; i < length; i++)
	{
		vector_get(importanceChangeThrVec, i, &pid);
		switch(threadSet[pid].importance)
		{
			case IMPORTANCE_LOW:
				ret = setpriority(PRIO_PROCESS, pid, 10);
				break;
			case IMPORTANCE_MID:
				ret = setpriority(PRIO_PROCESS, pid, 0);
				break;
			case IMPORTANCE_HIGH:
				ret = setpriority(PRIO_PROCESS, pid, -10);
				break;
			default:
				fprintf(stderr, "unknown importance %d\n", threadSet[pid].importance);
				destruction();
				exit(EXIT_FAILURE);
				break;
		}
		if(ret)
			fprintf(stderr, "thread %d disappeared...\n", pid);
		else
			INFO(("pid = %d, nice = %d\n", pid, (int)threadSet[pid].importance * -10));
	}
}
