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
 * @file core.h
 * @brief header file that control cpu core
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#ifndef __CORE_H__
#define __CORE_H__
#define MAX_FREQ 20
#define GOVERNOR_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define FREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
#define FREQ_TABLE_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies"
#define FREQ_TABLE_TIME_IN_STATE_PATH "/sys/devices/system/cpu/cpu0/cpufreq/stats/time_in_state"
#define FREQ_SET_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed"
#define CPUINFO_PATH "/proc/stat"
#include "vector.h"
#include <stdbool.h>

typedef struct _COREATTR
{
	bool online;
	int sumOfImportance;
	int numOfThreads;
	int numOfMidThreads;
	int numOfRunningThreads;
	int numOfRunningMidThreads;
	unsigned long long busy;
	unsigned long long nice_busy; // count only busy time with processes' nice value > 0
	unsigned long long idle;
	float util; // must be normalized to max level of frequency
	float midUtil; // ignore low importance thread's util
} COREATTR;

void initialize_cores(void);
void DPM(void);
void assign_core(int pid, int coreId, bool firstAssign);
void updateMinMaxCore(void);
void DVFS(void);
void setFreq(int freq);
void migration(void);
#endif // __CORE_H__
