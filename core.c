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
 * @version 20130409
 * @date 2013-04-09
 */
#include "core.h"
#include "config.h"
#include "importance.h"
#include "thread.h"
#include "common.h"
#include "debug.h"
#include "my_sysinfo.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/syscall.h> // syscall()
#include <unistd.h>

extern THREADATTR threadSet[];
extern COREATTR* coreSet;
extern vector **pidListVec;
extern bool touchIsOn;
extern float elapseTime;
extern int curFreq;
extern int maxFreq;
extern int minUtilCoreId;
extern int maxUtilCoreId;
extern int minImpCoreId;
extern int maxImpCoreId;

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
 * @brief record history number of process running
 */
static int procRunningHistory[CONFIG_NUM_OF_PROCESS_RUNNING_HISTORY_ENTRIES];

/**
 * @brief record last entry index in procRunningHistory
 */
static int procRunningIndex = 0;

/**
 * @brief summation of number of process running of all history entries
 */
static int sumProcRunning = 0;

/**
 * @brief average value of number of process running
 */
static int avgProcRunning = 0;

/**
 * @brief record history utilization for fine-grained thread-level dpm
 */
static float utilHistory[CONFIG_NUM_OF_HISTORY_ENTRIES];

/**
 * @brief record last entry index in utilHistory array
 */
static int utilHistoryIndex = 0;

/**
 * @brief summation of utilization history
 */
static float sumUtil = 0;

/**
 * @brief average value of utilization history
 */
static float avgUtil = 0;

/**
 * @brief record history mid utilization for fine-grained thread-level dvfs
 */
static float midUtilHistory[CONFIG_NUM_OF_HISTORY_ENTRIES];

/**
 * @brief record last entry index in midUtilHistory array
 */
static int midUtilHistoryIndex = 0;

/**
 * @brief average value of mid utilization history
 */
static float avgMidUtil = 0;

/**
 * @brief number of cores that are currently online
 */
static int numOfCoresOnline = 0;

/**
 * @brief frequency threshold to turn on 2, 3, 4 cores
 */
static int Thres[3] = {CONFIG_THRESHOLD2, CONFIG_THRESHOLD3, CONFIG_THRESHOLD4};

// frequency
static void getFrequencyTable(void);
static int getCurFreq(void);
static int getProperFreq(float util);
static void changeGovernorToUserspace(void);
// process
static int getNumProcessRunning(void);
// core
static void initialize_core(int coreId);
static int getFirstOfflineCore(void);
static void setCore(int coreId, bool turnOn);
static int getMinImpCoreExcept0(void);
static void balanceCoreImp(int minCoreId, int maxCoreId);
static void balanceCoreUtil(int minCoreId, int maxCoreId);
static void PourCoreImpUtil(int minCoreId);

/**
 * @brief initialize_cores initialize cores
 */
void initialize_cores(void)
{
	int i;
	char buff[BUFF_SIZE];
	FILE *fp;
	INFO(("initialize cores\n"));

	// core
	for(i = 0; i < my_get_nprocs_conf(); i++)
	{
		// open all cores temporary to know each core's busy and idle
		sprintf(buff, "/sys/devices/system/cpu/cpu%d/online", i);
		fp = fopen(buff, "w");
		if(fp)
		{
			fprintf(fp, "1\n");
			fclose(fp);
		}
		initialize_core(i);
	}
	numOfCoresOnline = my_get_nprocs_conf();
	
	// frequency
	getFrequencyTable();
	if(CONFIG_TURN_ON_DVFS)
	{
		changeGovernorToUserspace();
	}
	curFreq = getCurFreq();
	INFO(("max frequency = %d\n", maxFreq));

	for(i = 0; i < CONFIG_NUM_OF_PROCESS_RUNNING_HISTORY_ENTRIES; i++)
		procRunningHistory[i] = 1;
	sumProcRunning = CONFIG_NUM_OF_PROCESS_RUNNING_HISTORY_ENTRIES;
	memset(midUtilHistory, 0, sizeof(float) * CONFIG_NUM_OF_HISTORY_ENTRIES);
	memset(utilHistory, 0, sizeof(float) * CONFIG_NUM_OF_HISTORY_ENTRIES);
}

static void initialize_core(int coreId)
{
	int curCoreId;
	FILE *fp;
	char buff[BUFF_SIZE];
	unsigned long long cpuinfo[10];

	memset(&coreSet[coreId], 0, sizeof(COREATTR));
	coreSet[coreId].online = true;
	vector_remove_all(pidListVec[coreId]);

	// busy, idle
	fp = fopen(CPUINFO_PATH, "r");
	if(fp)
	{
		fgets(buff, BUFF_SIZE, fp); // ignore first line
		while(fgets(buff, BUFF_SIZE, fp))
		{
			sscanf(buff, "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &curCoreId, &cpuinfo[0], &cpuinfo[1], &cpuinfo[2], &cpuinfo[3], &cpuinfo[4], &cpuinfo[5], &cpuinfo[6], &cpuinfo[7], &cpuinfo[8], &cpuinfo[9]); // time(unit: jiffies) spent of all cpus for: user nice system idle iowait irq softirq stead guest
			if(curCoreId == coreId)
			{
				coreSet[coreId].busy = cpuinfo[0] + cpuinfo[1] + cpuinfo[2] + cpuinfo[4] + cpuinfo[5] + cpuinfo[6] + cpuinfo[7] + cpuinfo[8] + cpuinfo[9];
				coreSet[coreId].nice_busy = cpuinfo[1];
				coreSet[coreId].idle = cpuinfo[3];
				break;
			}
		}
		fclose(fp);
	}
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
	
	// get number of process running and update table
	numOfProcessRunning = getNumProcessRunning();
	if(numOfProcessRunning < 1) // something wrong
		numOfProcessRunning = 1; // at least our process is running

	procRunningIndex = (procRunningIndex + 1) % CONFIG_NUM_OF_PROCESS_RUNNING_HISTORY_ENTRIES;
	sumProcRunning -= procRunningHistory[procRunningIndex];
	procRunningHistory[procRunningIndex] = numOfProcessRunning;
	sumProcRunning += numOfProcessRunning;
	avgProcRunning = sumProcRunning / CONFIG_NUM_OF_PROCESS_RUNNING_HISTORY_ENTRIES;

	// get utilization sum and update table
	for(i = 0; i < my_get_nprocs_conf(); i++)
	{
		utilSum += coreSet[i].util;
	}

	utilHistoryIndex = (utilHistoryIndex + 1) % CONFIG_NUM_OF_HISTORY_ENTRIES;
	sumUtil -= utilHistory[utilHistoryIndex];
	utilHistory[utilHistoryIndex] = utilSum;
	sumUtil += utilSum;
	avgUtil = sumUtil / CONFIG_NUM_OF_HISTORY_ENTRIES;
	INFO(("utilization sum = %f\n", utilSum));

	// get number of cores that should online
	for(i = 0; i < my_get_nprocs_conf() - 1; i++)
	{
		//if(Thres[i] * (i+1) > utilSum)
		if(Thres[i] > avgUtil)
		{
			break;
		}
	}

	// if there are not enough processes are running, more cores are useless
	numOfCoresShouldOpen = i+1 > avgProcRunning ? avgProcRunning:i+1;
	INFO(("should open %d core, currently open %d core\n", numOfCoresShouldOpen, numOfCoresOnline));

	// setup core
	numOfCoresShouldOpen -= numOfCoresOnline;
	while(numOfCoresShouldOpen > 0) // we have to turn on core
	{
		INFO(("Trying to open cores\n"));
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
		minCoreId = getMinImpCoreExcept0();
		// dispatch threads to other cores
		if(minCoreId > 0)
		{
			PourCoreImpUtil(minCoreId);
			setCore(minCoreId, false);
			updateMinMaxCore();
		}
		numOfCoresShouldOpen++;
	}

	updateMinMaxCore();
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
		if(threadSet[pid].importance >= IMPORTANCE_MID)
			coreSet[coreId].numOfMidThreads++;
	}
	else
	{
		oldCoreId = threadSet[pid].coreId;

		coreSet[oldCoreId].sumOfImportance -= threadSet[pid].importance;
		coreSet[coreId].sumOfImportance += threadSet[pid].importance;

		coreSet[oldCoreId].numOfThreads--;
		coreSet[coreId].numOfThreads++;

		if(threadSet[pid].importance >= IMPORTANCE_MID)
		{
			coreSet[oldCoreId].numOfMidThreads--;
			coreSet[coreId].numOfMidThreads++;
		}

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
 * @brief updateMinMaxCore update minimal/maximum util/imp core
 */
void updateMinMaxCore(void)
{
	int i;
	minUtilCoreId = 0;
	maxUtilCoreId = 0;
	minImpCoreId = 0;
	maxImpCoreId = 0;

	for(i = 1; i < my_get_nprocs_conf(); i++)
	{
		if(coreSet[i].online)
		{
			if(coreSet[i].sumOfImportance < coreSet[minImpCoreId].sumOfImportance)
				minImpCoreId = i;
			if(coreSet[i].sumOfImportance > coreSet[maxImpCoreId].sumOfImportance)
				maxImpCoreId = i;
			if(coreSet[i].util < coreSet[minUtilCoreId].util)
				minUtilCoreId = i;
			if(coreSet[i].util > coreSet[maxUtilCoreId].util)
				maxUtilCoreId = i;
		}
	}
}

/**
 * @brief DVFS tune cpu frequency to specific level based on utilization of mid importance thread
 */
void DVFS(void)
{
	float weight, curUtil;
	if(!CONFIG_TURN_ON_DVFS)
		return;
	INFO(("DVFS\n"));

	if(touchIsOn)
	{
		INFO(("touchIsOn, ignore DVFS\n"));
		return; // already early set freq to max
	}

	// record utilization in history table
	midUtilHistoryIndex = (midUtilHistoryIndex + 1) % CONFIG_NUM_OF_HISTORY_ENTRIES;
	weight = 1 + (float)(coreSet[maxUtilCoreId].numOfRunningThreads - coreSet[maxUtilCoreId].numOfRunningMidThreads) / ((coreSet[maxUtilCoreId].numOfThreads - coreSet[maxUtilCoreId].numOfMidThreads) + coreSet[maxUtilCoreId].numOfRunningMidThreads * CONFIG_MID_WEIGHT_OVER_LOW);
	curUtil = coreSet[maxUtilCoreId].midUtil * weight;
	if(curUtil > coreSet[maxUtilCoreId].util)
		curUtil = coreSet[maxUtilCoreId].util;

	DVFS_INFO(("before weight: %f, weight: %f, after weight: %f\n", coreSet[maxUtilCoreId].midUtil, weight, curUtil));
	avgMidUtil = avgMidUtil + (curUtil - midUtilHistory[midUtilHistoryIndex]) / CONFIG_NUM_OF_HISTORY_ENTRIES;
	midUtilHistory[midUtilHistoryIndex] = curUtil;

	curFreq = getProperFreq(avgMidUtil);
	DVFS_INFO(("set frequency to %d\n", curFreq));
	setFreq(curFreq);
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
		ERR(("cannot set frequency\n"));
		destruction();
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief migration load balance importance sum and utilization sum among cores
 */
void migration(void)
{
	if(!CONFIG_TURN_ON_MIGRATION)
		return;
	INFO(("migration\n"));

	if(numOfCoresOnline == 1)
		return;

	// balance importance among cores
	balanceCoreImp(minImpCoreId, maxImpCoreId);
	updateMinMaxCore();
	
	// balance utilization among cores
	balanceCoreUtil(minUtilCoreId, maxUtilCoreId);
	updateMinMaxCore();
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
	char tmpstr[1024] = "";
	
	numOfFreq = 0;
	if(CONFIG_USE_TIME_IN_STATE_FREQUENCY_TABLE)
	{
		fp = fopen(FREQ_TABLE_TIME_IN_STATE_PATH, "r");
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
			ERR(("cannot get frequency table\n"));
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
			ERR(("cannot get frequency table\n"));
			destruction();
			exit(EXIT_FAILURE);
		}
	}
	
	maxFreq = freqTable[numOfFreq -1];

	for(i = 0; i < numOfFreq; i++)
	{
		sprintf(buff, "%llu ", freqTable[i]);
		strcat(tmpstr, buff);
	}
	INFO(("%s\n", tmpstr));
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
		ERR(("cannot fetch current frequency\n"));
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
	int candidateFreqLevel = 0;
	bool fastUp = false;
	static int up_counter = 0;
	static int lastFreqLevel = 0;

	if(midUtilHistory[midUtilHistoryIndex] > midUtilHistory[(midUtilHistoryIndex - 1) % CONFIG_NUM_OF_HISTORY_ENTRIES])
		up_counter++;
	else
		up_counter = 0;
	if(up_counter >= 2)
	{
		up_counter = 1;
		fastUp = true;
	}

	if(fastUp)
	{
		candidateFreqLevel = (lastFreqLevel + numOfFreq - 1) / 2;
	}
	else
	{
		for(candidateFreqLevel = 0; candidateFreqLevel < numOfFreq - 1; candidateFreqLevel++)
		{
			if(freqTable[candidateFreqLevel] > util)
			{
				if(freqTable[candidateFreqLevel] * CONFIG_SCALE_UP_RATIO < util)
				{
					candidateFreqLevel++;
				}
				break;
			}
		}
	}

	lastFreqLevel = candidateFreqLevel;
	return freqTable[candidateFreqLevel];
}

/**
 * @brief changeGovernorToUserspace change cpu governor to userspace and set frequency to highest
 */
static void changeGovernorToUserspace()
{
	errno = 0;
	char buff[BUFF_SIZE];
	FILE *fp = fopen(GOVERNOR_PATH, "w");
	if(fp)
	{
		fprintf(fp, "userspace\n");
		fclose(fp);
	}
	else
	{
		ERR(("cannot set governor to userspace\n"));
		sprintf(buff, "Error %d \n", errno);
		ERR((buff));
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
		ERR(("cannot fetch cuurent number of running\n"));
		destruction();
		exit(EXIT_FAILURE);
	}
	INFO(("process running = %d\n", numOfProcessRunning));
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
	for(candidateCoreId = 1; candidateCoreId < my_get_nprocs_conf(); candidateCoreId++)
	{
		if(!coreSet[candidateCoreId].online)
		{
			break;
		}
	}
	if(candidateCoreId == my_get_nprocs_conf())
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
			numOfCoresOnline++;
			initialize_core(coreId);
		}
		else
		{
			fprintf(fp, "0\n");
			numOfCoresOnline--;
			memset(&coreSet[coreId], 0, sizeof(COREATTR));
			vector_remove_all(pidListVec[coreId]);
		}
		fclose(fp);
	}
	else
	{
		ERR(("cannot set core %d on\n", coreId));
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
static int getMinImpCoreExcept0(void)
{
	int i, minCoreId = -1;

	if(minImpCoreId != 0)
		return minImpCoreId;

	for(i = 1; i < my_get_nprocs_conf(); i++)
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
 * @brief balanceCoreImp balance two cores' importance sum, search all pids, find mid importance thread on maximum sum core and move it to minimum sum core
 *
 * @param minCoreId
 * @param maxCoreId
 */
static void balanceCoreImp(int minCoreId, int maxCoreId)
{
	int i = 0, length, pid;
	int avgImportance = (coreSet[minCoreId].sumOfImportance + coreSet[maxCoreId].sumOfImportance) / 2;
	INFO(("balance imp from max core %d imp %d to min core %d imp %d\n", maxCoreId, coreSet[maxCoreId].sumOfImportance, minCoreId, coreSet[minCoreId].sumOfImportance));

	if(minCoreId == maxCoreId)
	{
		INFO(("no need to balance\n"));
		return;
	}

	length = vector_length(pidListVec[maxCoreId]);
	if(length != 0)
	{
		pid = ((int *)pidListVec[maxCoreId]->elems)[0];
		for(i = 0; i < length && coreSet[minCoreId].sumOfImportance + threadSet[pid].importance < avgImportance; i++)
		{
			pid = ((int *)pidListVec[maxCoreId]->elems)[i];
			vector_push(pidListVec[minCoreId], &pid);
			assign_core(pid, minCoreId, false);
		}
		vector_remove_some(pidListVec[maxCoreId], 0, i);
	}
}

/**
 * @brief balanceCoreUtil balance two cores' utilization sum, search all pids, find low importance thread on maximum sum core and move it to minimum sum core
 *
 * @param minCoreId
 * @param maxCoreId
 */
static void balanceCoreUtil(int minCoreId, int maxCoreId)
{
	int i = 0, length, pid;
	int halfUtil = (coreSet[minCoreId].util + coreSet[maxCoreId].util) / 2;
	INFO(("balance util from max core %d util %f to min core %d util %f\n", maxCoreId, coreSet[maxCoreId].util, minCoreId, coreSet[minCoreId].util));

	if(minCoreId == maxCoreId)
	{
		INFO(("no need to balance\n"));
		return;
	}
	
	length = vector_length(pidListVec[maxCoreId]);
	if(length != 0)
	{
		pid = ((int *)pidListVec[maxCoreId]->elems)[0];
		for(i = 0; i < length && coreSet[minCoreId].util + threadSet[pid].util < halfUtil; i++)
		{
			pid = ((int *)pidListVec[maxCoreId]->elems)[i];
			vector_push(pidListVec[minCoreId], &pid);
			assign_core(pid, minCoreId, false);
		}
		vector_remove_some(pidListVec[maxCoreId], 0, i);
	}
}

/**
 * @brief PourCoreImpUtil when a core turn off, migrate all threads on this core to other cores, migrate low importance threads based on utilization sum, migrate mid/high importance threads based on importance sum
 *
 * @param victimCoreId
 */
static void PourCoreImpUtil(int victimCoreId)
{
	int i, length, pid;
	int localMinImpCoreId = 0, localMinUtilCoreId = 0;
	INFO(("move threads away from core %d\n", victimCoreId));

	for(i = 1; i < my_get_nprocs_conf(); i++)
	{
		if(coreSet[i].online && i != victimCoreId)
		{
			if(coreSet[i].sumOfImportance < coreSet[localMinImpCoreId].sumOfImportance)
				localMinImpCoreId = i;
			if(coreSet[i].util < coreSet[localMinUtilCoreId].util)
				localMinUtilCoreId = i;
		}
	}

	length = vector_length(pidListVec[victimCoreId]);
	for(i = 0; i < length; i++)
	{
		pid = ((int *)pidListVec[victimCoreId])[i];
		if(threadSet[pid].importance >= IMPORTANCE_MID) // use importance sum to determine which core to pour
			assign_core(pid, localMinImpCoreId, false);
		else // importance = IMPORTANCE_LOW, use utilization sum to determine which core to pour
			assign_core(pid, localMinUtilCoreId, false);
	}
	vector_remove_all(pidListVec[victimCoreId]);
}
