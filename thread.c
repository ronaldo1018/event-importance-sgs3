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
extern float elapseTime;
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

	if((d = opendir("/proc")) != NULL)
	{
		while((de = readdir(d)) != 0)
		{
			if(isdigit(de->d_name[0]))
			{
				pid = atoi(de->d_name);
				initialize_thread(pid);
				if(threadSet[pid].isSysThr)
					change_importance(pid, IMPORTANCE_MID, true); // set to mid because we can get utilization of this thread for dvfs, dpm, ....
				else
					change_importance(pid, IMPORTANCE_LOW, true);
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
	
	//parseString(buff, 1, threadSet[pid].name, &failbit);
	//parseInt2(buff, 18, &threadSet[pid].originalNice, 38, &threadSet[pid].coreId, &failbit);
	// get utime and stime
	parseInt2(buff, 13, &threadSet[pid].oldutime, 14, &threadSet[pid].oldstime, &failbit);
	INFO(("initialize thread %d: s:%s c:%d ut:%d st:%d\n", pid, threadSet[pid].isSysThr?"s":"u", threadSet[pid].coreId, threadSet[pid].oldutime, threadSet[pid].oldstime));
	if(failbit) return; // thread disappeared..
	threadSet[pid].isUsed = 1;
	//assign_core(pid, threadSet[pid].coreId, true);
	assign_core(pid, 0, true);
	threadSet[pid].coreId = 0;
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
}

/**
 * @brief calculate_utilization calculate each thread's new utilization
 */
void calculate_utilization(void)
{
	int i;
	DIR *d;
	struct dirent *de;
	FILE *fp;
	bool failbit;
	int pid;
	int coreId;
	char buff[BUFF_SIZE];
	struct timeval t;
	struct timezone timez;
	unsigned long long cpuinfo[10], busy, nice_busy, idle, busysub, nice_busysub, idlesub;
	int newExecTime;
	DVFS_INFO(("calculate utilization\n"));

	// clear coreSet util
	for(i = 0; i < CONFIG_NUM_OF_CORE; i++)
	{
		coreSet[i].execTime = 0;
		coreSet[i].midExecTime = 0;
		coreSet[i].util = 0;
		coreSet[i].midUtil = 0;
	}

	// update timestamp
	gettimeofday(&t, &timez);
	elapseTime = t.tv_sec - oldTime.tv_sec + (float)(t.tv_usec - oldTime.tv_usec) / 1000000.0;
	DVFS_INFO(("elapseTime = %f\n", elapseTime));
	oldTime.tv_sec = t.tv_sec;
	oldTime.tv_usec = t.tv_usec;

	if(CONFIG_FAST_MID_UTIL_ENABLED)
	{
		INFO(("fast mid util enabled\n"));
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
					coreSet[coreId].util = (float)(busysub) / (busysub + idlesub) * curFreq;
					coreSet[coreId].midUtil = (float)(busysub - nice_busysub) / (busysub + idlesub) * curFreq;
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
	}
	else
	{
		// update each thread's execution time
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
						// we need to add some compensation time to the core this thread was running on to precisely estimate running time
						coreId = threadSet[pid].coreId;
						coreSet[coreId].execTime++;
						if(threadSet[pid].importance >= IMPORTANCE_MID)
						{
							coreSet[coreId].midExecTime++;
						}
						continue;
					}

					// notice: curFreq may change between two utilization samplings, but we use last value to estimate for simplicity
					newExecTime = threadSet[pid].utime - threadSet[pid].oldutime + threadSet[pid].stime - threadSet[pid].oldstime;
					if(newExecTime != 0)
						DVFS_INFO(("pid %d in core %d has exec time %d\n", pid, threadSet[pid].coreId, threadSet[pid].utime - threadSet[pid].oldutime + threadSet[pid].stime - threadSet[pid].oldstime));
					coreSet[threadSet[pid].coreId].execTime += newExecTime;
					if(threadSet[pid].importance >= IMPORTANCE_MID)
						coreSet[threadSet[pid].coreId].midExecTime += newExecTime;
					threadSet[pid].execTime = newExecTime;
					threadSet[pid].oldutime = threadSet[pid].utime;
					threadSet[pid].oldstime = threadSet[pid].stime;
				}
			}
			closedir(d);

			// update core's execution time
			for(i = 0; i < CONFIG_NUM_OF_CORE; i++)
			{
				coreSet[i].util = execTimeToUtil(coreSet[i].execTime);
				coreSet[i].midUtil = execTimeToUtil(coreSet[i].midExecTime);
			}
		}
		else
		{
			fprintf(stderr, "cannot open directory /proc\n");
		}
	}
}

/**
 * @brief execTimeToUtil convert a thread's execution time to utilization based on current elaspe time and frequency
 *
 * @param execTime
 */
float execTimeToUtil(int execTime)
{
	return (float)execTime / CONFIG_CORE_UPDATE_TIME / elapseTime * curFreq;
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
		for(i = 1; i < CONFIG_NUM_OF_CORE; i++)
		{
			if(coreSet[i].sumOfImportance < coreSet[minCoreId].sumOfImportance)
			{
				minCoreId = i;
			}
		}
	}
	else // importance = low
	{
		for(i = 1; i < CONFIG_NUM_OF_CORE; i++)
		{
			if(coreSet[i].execTime < coreSet[minCoreId].execTime)
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
				ret = setpriority(PRIO_PROCESS, pid, CONFIG_NICE_LOW);
				break;
			case IMPORTANCE_MID:
				ret = setpriority(PRIO_PROCESS, pid, CONFIG_NICE_MID);
				break;
			case IMPORTANCE_HIGH:
				ret = setpriority(PRIO_PROCESS, pid, CONFIG_NICE_HIGH);
				break;
			default:
				fprintf(stderr, "unknown importance %d\n", threadSet[pid].importance);
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
