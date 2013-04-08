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
 * @file core.c
 * @brief control cpu core
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130317
 * @date 2013-03-17
 */
#include "core.h"
#include "config.h"
#include "importance.h"
#include "thread.h"
#include "common.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/syscall.h> // syscall()

extern THREADATTR threadSet[];
extern COREATTR coreSet[];
extern bool touchIsOn;
extern int curFreq;
extern int maxFreq;

// variables or functions only used in this file
/**
 * @brief frequency tables
 */
static unsigned long long freqTable[MAX_FREQ];

/**
 * @brief number of frequencies available
 */
static int numOfFreq = 0;

/**
 * @brief number of cores that are currently online
 */
static int numOfCoresOnline = 0;

/**
 * @brief frequency threshold to turn on 2, 3, 4 cores
 */
static int Thres[3] = {THRESHOLD2, THRESHOLD3, THRESHOLD4};

// frequency
static void getFrequencyTable(void);
static int getCurFreq(void);
static int getProperFreq(float util);
static void changeGovernorToUserspace(void);
// process
static int getNumProcessRunning(void);
// core
static int getFirstOfflineCore(void);
static void setCore(int coreId, bool turnOn);
static int getMinImpCore(bool exceptCore0);
static int getMaxImpCore(void);
static void balanceCoreImp(int minCoreId, int maxCoreId);
static int getMinUtilCore(void);
static int getMaxUtilCore(void);
static void balanceCoreUtil(int minCoreId, int maxCoreId);
static void PourCoreImpUtil(int minCoreId);

/**
 * @brief initialize_cores initialize cores
 */
void initialize_cores(void)
{
	int i, coreId;
	char buff[BUFF_SIZE];
	unsigned long long cpuinfo[10];
	FILE *fp;
	INFO(("initialize cores\n"));

	memset(coreSet, 0, sizeof(COREATTR) * NUM_OF_CORE);
	
	// core
	for(i = 0; i < NUM_OF_CORE; i++)
	{
		// open all cores temporary to know each core's busy and idle
		sprintf(buff, "/sys/devices/system/cpu/cpu%d/online", i);
		fp = fopen(buff, "w");
		if(fp)
		{
			fprintf(fp, "1\n");
			coreSet[i].online = true;
			fclose(fp);
		}
	}
	numOfCoresOnline = NUM_OF_CORE;

	// busy, idle
	fp = fopen(CPUINFO_PATH, "r");
	if(fp)
	{
		fgets(buff, BUFF_SIZE, fp); // ignore first line
		while(fgets(buff, BUFF_SIZE, fp))
		{
			sscanf(buff, "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &coreId, &cpuinfo[0], &cpuinfo[1], &cpuinfo[2], &cpuinfo[3], &cpuinfo[4], &cpuinfo[5], &cpuinfo[6], &cpuinfo[7], &cpuinfo[8], &cpuinfo[9]); // time(unit: jiffies) spent of all cpus for: user nice system idle iowait irq softirq stead guest
			coreSet[coreId].busy = cpuinfo[0] + cpuinfo[1] + cpuinfo[2] + cpuinfo[4] + cpuinfo[5] + cpuinfo[6] + cpuinfo[7] + cpuinfo[8] + cpuinfo[9];
			coreSet[coreId].idle = cpuinfo[3];
			
			if(coreId == NUM_OF_CORE - 1)
				break;
		}
		fclose(fp);
	}
	
	// frequency
	getFrequencyTable();
	if(CONFIG_TURN_ON_DVFS)
	{
		changeGovernorToUserspace();
	}
	curFreq = getCurFreq();
	INFO(("max frequency = %d\n", maxFreq));
}

/**
 * @brief check_core_util_up if any core's utilization is above a ratio, then need change frequency immediately
 */
bool check_core_util_up(void)
{
	int coreId;
	char buff[BUFF_SIZE];
	unsigned long long cpuinfo[10], busy, idle;
	float util;
	FILE *fp;

	fp = fopen(CPUINFO_PATH, "r");
	if(fp)
	{
		fgets(buff, BUFF_SIZE, fp); // ignore first line
		while(fgets(buff, BUFF_SIZE, fp))
		{
			if(strstr(buff, "cpu"))
			{
				sscanf(buff, "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &coreId, &cpuinfo[0], &cpuinfo[1], &cpuinfo[2], &cpuinfo[3], &cpuinfo[4], &cpuinfo[5], &cpuinfo[6], &cpuinfo[7], &cpuinfo[8], &cpuinfo[9]); // time(unit: jiffies) spent of all cpus for: user nice system idle iowait irq softirq stead guest
				busy = cpuinfo[0] + cpuinfo[1] + cpuinfo[2] + cpuinfo[4] + cpuinfo[5] + cpuinfo[6] + cpuinfo[7] + cpuinfo[8] + cpuinfo[9] - coreSet[coreId].busy;
				idle = cpuinfo[3] - coreSet[coreId].idle;
				util = (float)busy / (busy + idle);
				if(util > CONFIG_SCALE_UP_RATIO)
				{
					INFO(("***************************need scale up, early DVFS*************************************\n"));
					fclose(fp);
					return true;
				}
			}
			else
			{
				break;
			}
		}
		fclose(fp);
	}
	return false;
}

/**
 * @brief DPM turn on/off cores based on current utilization sum
 */
void DPM(void)
{
	int i, numOfCoresShouldOpen;
	int candidateCoreId, minCoreId;
	int numOfProcessRunning;
	float utilSum = 0;
	if(!CONFIG_TURN_ON_DPM)
		return;
	INFO(("DPM\n"));
	
	// get number of process running
	numOfProcessRunning = getNumProcessRunning();
	if(numOfProcessRunning < 0) // something error
		numOfProcessRunning = 1; // at least our process is running

	// get utilization sum
	for(i = 0; i < NUM_OF_CORE; i++)
	{
		utilSum += coreSet[i].util;
	}
	INFO(("utilization sum = %f\n", utilSum));

	// get number of cores that should online
	for(i = 0; i < NUM_OF_CORE - 1; i++)
	{
		if(Thres[i] > utilSum)
		{
			break;
		}
	}

	// if there are not enough processes are running, more cores are useless
	numOfCoresShouldOpen = i+1 > numOfProcessRunning ? numOfProcessRunning:i+1;
	INFO(("should open %d core, currently open %d core\n", numOfCoresShouldOpen, numOfCoresOnline));

	// setup core
	numOfCoresShouldOpen -= numOfCoresOnline;
	while(numOfCoresShouldOpen > 0) // we have to turn on core
	{
		// find candidate core that is not online now
		candidateCoreId = getFirstOfflineCore();
		if(candidateCoreId != -1)
		{
			setCore(candidateCoreId, true);
			// note: thread migration will be done in migration()
			numOfCoresShouldOpen--;
		}
	}
	while(numOfCoresShouldOpen < 0) // we have to turn off core
	{
		// find min importance sum core
		minCoreId = getMinImpCore(true);
		// dispatch threads to other cores
		PourCoreImpUtil(minCoreId);
		setCore(minCoreId, false);
		numOfCoresShouldOpen++;
	}
}

/**
 * @brief assign_core assign a thread to specific core and update core information
 *
 * @param pid
 * @param coreId
 * @param firstAssign
 */
void assign_core(int pid, int coreId, bool firstAssign)
{
	int oldCoreId;
	int mask;
	INFO(("pid %d assign to core %d\n", pid, coreId));

	if(firstAssign)
	{
		coreSet[coreId].sumOfImportance += threadSet[pid].importance;
		coreSet[coreId].numOfThreads++;
		coreSet[coreId].util += threadSet[pid].util;
		if(threadSet[pid].importance >= IMPORTANCE_MID)
			coreSet[coreId].midUtil += threadSet[pid].util;
	}
	else
	{
		oldCoreId = threadSet[pid].coreId;

		coreSet[oldCoreId].sumOfImportance -= threadSet[pid].importance;
		coreSet[coreId].sumOfImportance += threadSet[pid].importance;

		coreSet[oldCoreId].numOfThreads--;
		coreSet[coreId].numOfThreads++;

		coreSet[oldCoreId].util -= threadSet[pid].util;
		coreSet[coreId].util += threadSet[pid].util;

		if(threadSet[pid].importance >= IMPORTANCE_MID)
		{
			coreSet[oldCoreId].midUtil -= threadSet[pid].util;
			coreSet[coreId].midUtil += threadSet[pid].util;
		}
	}
	threadSet[pid].coreId = coreId;
	mask = 1 << coreId;
	syscall(__NR_sched_setaffinity, pid, sizeof(mask), &mask);
}

/**
 * @brief DVFS tune cpu frequency to specific level based on utilization of mid importance thread
 */
void DVFS(void)
{
	int i, maxCoreId;
	if(!CONFIG_TURN_ON_DVFS)
		return;
	INFO(("DVFS\n"));
	DVFS_INFO(("core %d has midutil %f\n", 0, coreSet[0].midUtil));

	if(touchIsOn)
	{
		setFreq(maxFreq);
	}
	else
	{
		maxCoreId = 0;
		for(i = 1; i < NUM_OF_CORE; i++)
		{
			DVFS_INFO(("core %d has midutil %f\n", i, coreSet[i].midUtil));
			if(coreSet[i].midUtil > coreSet[maxCoreId].midUtil)
			{
				maxCoreId = i;
			}
		}
		curFreq = getProperFreq(coreSet[maxCoreId].midUtil);
		DVFS_INFO(("max mid util = %f\n", coreSet[maxCoreId].midUtil));
		DVFS_INFO(("set frequency to %d\n", curFreq));
		setFreq(curFreq);
	}
}

/**
 * @brief setFreq set cpu frequency by sysfs
 *
 * @param freq target cpu frequency
 */
void setFreq(int freq)
{
	FILE *fp = fopen(FREQ_SET_PATH, "w");
	if(fp)
	{
		fprintf(fp, "%d\n", freq);
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot set frequency\n");
		destruction();
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief migration load balance importance sum and utilization sum among cores
 */
void migration(void)
{
	int minCoreId, maxCoreId;
	if(!CONFIG_TURN_ON_MIGRATION)
		return;
	INFO(("migration\n"));

	// balance importance among cores
	// find min importance sum core
	minCoreId = getMinImpCore(false);
	// find max importance sum core
	maxCoreId = getMaxImpCore();
	balanceCoreImp(minCoreId, maxCoreId);
	
	// balance utilization among cores
	// find min utilization sum core
	minCoreId = getMinUtilCore();
	// find max utilization sum core
	maxCoreId = getMaxUtilCore();
	balanceCoreUtil(minCoreId, maxCoreId);
}

/**
 * @brief getFrequencyTable store all available frequencies in freqTable
 */
static void getFrequencyTable(void)
{
	int i;
	unsigned long long tmp;
	FILE *fp;
	char buff[BUFF_SIZE];
	
	numOfFreq = 0;
	if(CONFIG_IS_SAMSUNG_FREQUENCY_TABLE)
	{
		fp = fopen(FREQ_TABLE_SAMSUNG_PATH, "r");
		if(fp)
		{
			while(fgets(buff, BUFF_SIZE, fp))
			{
				sscanf(buff, "%llu", &freqTable[numOfFreq]);
				numOfFreq++;
			}
			fclose(fp);

			// reverse array because original frequency order is decreasing
			for(i = 0; i < numOfFreq / 2; i++)
			{
				tmp = freqTable[i];
				freqTable[i] = freqTable[numOfFreq - i - 1];
				freqTable[numOfFreq - i - 1] = tmp;
			}
		}
		else
		{
			fprintf(stderr, "cannot get frequency table\n");
			destruction();
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		fp = fopen(FREQ_TABLE_PATH, "r");
		if(fp)
		{
			while(fscanf(fp, "%llu", &freqTable[numOfFreq]) > 0)
				numOfFreq++;
			fclose(fp);
		}
		else
		{
			fprintf(stderr, "cannot get frequency table\n");
			destruction();
			exit(EXIT_FAILURE);
		}
	}
	
	maxFreq = freqTable[numOfFreq -1];

	for(i = 0; i < numOfFreq; i++)
		INFO(("%llu ", freqTable[i]));
	INFO(("\n"));
}

/**
 * @brief getCurFreq get current cpu frequency
 *
 * @return current frequency
 */
static int getCurFreq(void)
{
	int freq;
	FILE *fp = fopen(FREQ_PATH, "r");
	if(fp)
	{
		fscanf(fp, "%d", &freq);
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot fetch cuurent frequency\n");
		destruction();
		exit(EXIT_FAILURE);
	}
	return freq;
}

/**
 * @brief getProperFreq use utilization sum to calculate which frequency level to use
 *
 * @param util utilization sum
 *
 * @return frequency level to use
 */
static int getProperFreq(float util)
{
	static int count = 0;
	static int lastFreqLevel = 0;
	int candidateFreqLevel = 0;

	for(candidateFreqLevel = 0; candidateFreqLevel < numOfFreq - 1; candidateFreqLevel++)
	{
		if(freqTable[candidateFreqLevel] * CONFIG_SCALE_UP_RATIO > util)
		{
			break;
		}
	}
	if(candidateFreqLevel >= lastFreqLevel)
	{
		count = 0;
		lastFreqLevel = candidateFreqLevel;
	}
	else
	{
		count++;
		if(count > 5)
		{
			lastFreqLevel = candidateFreqLevel > 0 ? candidateFreqLevel-1 : 0;
			count = 0;
		}
	}
	return freqTable[lastFreqLevel];
}

/**
 * @brief changeGovernorToUserspace change cpu governor to userspace and set frequency to highest
 */
static void changeGovernorToUserspace()
{
	FILE *fp = fopen(GOVERNOR_PATH, "w");
	if(fp)
	{
		fprintf(fp, "userspace\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot set governor to userspace\n");
		destruction();
		exit(EXIT_FAILURE);
	}
	setFreq(maxFreq);
}

/**
 * @brief getNumProcessRunning find current number of processes that are running
 *
 * @return number of processes in running state
 */
static int getNumProcessRunning(void)
{
	int numOfProcessRunning = -1;
	char buff[BUFF_SIZE];
	FILE *fp = fopen(CPUINFO_PATH, "r");
	if(fp)
	{
		while(fgets(buff, BUFF_SIZE, fp))
		{
			if(strstr(buff, "procs_running"))
			{
				sscanf(buff, "procs_running %d", &numOfProcessRunning);
				break;
			}
		}
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot fetch cuurent number of running\n");
		destruction();
		exit(EXIT_FAILURE);
	}
	return numOfProcessRunning;
}

/**
 * @brief getFirstOfflineCore find the first offline core
 *
 * @return the first offline core
 */
static int getFirstOfflineCore(void)
{
	int candidateCoreId;
	// core 0 never offline
	for(candidateCoreId = 1; candidateCoreId < NUM_OF_CORE; candidateCoreId++)
	{
		if(!coreSet[candidateCoreId].online)
		{
			break;
		}
	}
	if(candidateCoreId == NUM_OF_CORE)
	{
		return -1; // no offline core found
	}
	return candidateCoreId;
}

/**
 * @brief setCore turn on/off a core
 *
 * @param coreId the core to be turn on/off
 * @param turnOn true to turn on, false to turn off
 */
static void setCore(int coreId, bool turnOn)
{
	FILE *fp;
	char buff[BUFF_SIZE];
	INFO(("core %d is now %s\n", coreId, turnOn ? "online":"offline"));

	sprintf(buff, "/sys/devices/system/cpu/cpu%d/online", coreId);
	fp = fopen(buff, "w");
	if(fp)
	{
		if(turnOn)
		{
			fprintf(fp, "1\n");
			memset(&coreSet[coreId], 0, sizeof(COREATTR));
			coreSet[coreId].online = true;
			numOfCoresOnline++;
		}
		else
		{
			fprintf(fp, "0\n");
			coreSet[coreId].online = false;
			numOfCoresOnline--;
		}
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot set core %d on\n", coreId);
		destruction();
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief getMinImpCore return the core with the minimum importance sum
 *
 * @param exceptCore0 if true, then core 0 will not be considered even core 0 has least importance sum
 *
 * @return the core with the minimum importance sum, -1 on no such core exist
 */
static int getMinImpCore(bool exceptCore0)
{
	int i, minCoreId = -1;
	for(i = exceptCore0 ? 1:0; i < NUM_OF_CORE; i++)
	{
		if(coreSet[i].online)
		{
			if(minCoreId == -1 ||
				coreSet[i].sumOfImportance < coreSet[minCoreId].sumOfImportance)
			{
				minCoreId = i;
			}
		}
	}
	INFO(("min imp core is %d\n", minCoreId));
	return minCoreId;
}

/**
 * @brief getMaxImpCore return the core with the maximum importance sum
 *
 * @return the core with the maximum importance sum
 */
static int getMaxImpCore(void)
{
	int i, maxCoreId = 0;
	for(i = 1; i < NUM_OF_CORE; i++)
		if(coreSet[i].online && coreSet[i].sumOfImportance > coreSet[maxCoreId].sumOfImportance)
			maxCoreId = i;
	INFO(("max imp core is %d\n", maxCoreId));
	return maxCoreId;
}

/**
 * @brief balanceCoreImp balance two cores' importance sum, search all pids, find mid importance thread on maximum sum core and move it to minimum sum core
 *
 * @param minCoreId
 * @param maxCoreId
 */
static void balanceCoreImp(int minCoreId, int maxCoreId)
{
	int pid;
	int avgImportance = (coreSet[minCoreId].sumOfImportance + coreSet[maxCoreId].sumOfImportance) / 2;
	INFO(("balance imp from max core %d to min core %d\n", maxCoreId, minCoreId));

	if(minCoreId == maxCoreId)
	{
		INFO(("no need to balance\n"));
		return;
	}

	pid = 1;
	while(coreSet[minCoreId].sumOfImportance < avgImportance && pid <= PID_MAX)
	{
		if(threadSet[pid].isUsed && threadSet[pid].coreId == maxCoreId && threadSet[pid].importance >= IMPORTANCE_MID)
		{
			assign_core(pid, minCoreId, false);
		}
		pid++;
	}
}

/**
 * @brief getMinUtilCore return the core with minimum utilization sum
 *
 * @return the core with minimum utilization sum
 */
static int getMinUtilCore(void)
{
	int i, minCoreId = 0;
	for(i = 1; i < NUM_OF_CORE; i++)
		if(coreSet[i].online && coreSet[i].util < coreSet[minCoreId].util)
			minCoreId = i;
	INFO(("min util core is %d\n", minCoreId));
	return minCoreId;
}

/**
 * @brief getMaxUtilCore return the core with maximum utilization sum
 *
 * @return the core with maximum utilization sum
 */
static int getMaxUtilCore(void)
{
	int i, maxCoreId = 0;
	for(i = 1; i < NUM_OF_CORE; i++)
		if(coreSet[i].online && coreSet[i].util > coreSet[maxCoreId].util)
			maxCoreId = i;
	INFO(("max util core is %d\n", maxCoreId));
	return maxCoreId;
}

/**
 * @brief balanceCoreUtil balance two cores' utilization sum, search all pids, find low importance thread on maximum sum core and move it to minimum sum core
 *
 * @param minCoreId
 * @param maxCoreId
 */
static void balanceCoreUtil(int minCoreId, int maxCoreId)
{
	int i;
	int avgUtil = (coreSet[minCoreId].util + coreSet[maxCoreId].util) / 2;
	INFO(("balance util from max core %d to min core %d\n", maxCoreId, minCoreId));

	if(minCoreId == maxCoreId)
	{
		INFO(("no need to balance\n"));
		return;
	}
	
	i = 1;
	while(coreSet[minCoreId].util < avgUtil && i <= PID_MAX)
	{
		// only low importance thread will be moved
		if(threadSet[i].isUsed && threadSet[i].coreId == maxCoreId && threadSet[i].importance == IMPORTANCE_LOW)
		{
			assign_core(i, minCoreId, false);
		}
		i++;
	}
}

/**
 * @brief PourCoreImpUtil when a core turn off, migrate all threads on this core to other cores, migrate low importance threads based on utilization sum, migrate mid/high importance threads based on importance sum
 *
 * @param victimCoreId
 */
static void PourCoreImpUtil(int victimCoreId)
{
	int i, pid, minImpCoreId, minUtilCoreId;
	INFO(("move threads away from core %d\n", victimCoreId));

	minImpCoreId = 0;
	minUtilCoreId = 0;
	for(i = 1; i < NUM_OF_CORE; i++)
	{
		if(coreSet[i].online)
		{
			if(coreSet[i].sumOfImportance < coreSet[minImpCoreId].sumOfImportance)
			{
				minImpCoreId = i;
			}
			if(coreSet[i].util < coreSet[minUtilCoreId].util)
			{
				minUtilCoreId = i;
			}
		}
	}

	for(pid = 1; pid <= PID_MAX ; pid++)
	{
		if(threadSet[pid].isUsed && threadSet[pid].coreId == victimCoreId)
		{
			if(threadSet[pid].importance >= IMPORTANCE_MID) // use importance sum to determine which core to pour
			{
				assign_core(pid, minImpCoreId, false);
			}
			else // importance = IMPORTANCE_LOW, use utilization sum to determine which core to pour
			{
				assign_core(pid, minUtilCoreId, false);
			}
		}
	}
}
