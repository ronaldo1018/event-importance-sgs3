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
 * @file thread.h
 * @brief header file that control threads
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#ifndef __THREAD_H__
#define __THREAD_H__
#include "vector.h"
#include "importance.h"
#include <stdbool.h>

typedef struct _THREADATTR
{
	bool isUsed; // whether if there's a thread has this pid
	bool isSysThr; // whether if it's a system service or application
	enum IMPORTANCE_VALUE importance; // importance value of a thread(currently 0(low) or 1(mid) or 2(high))
	int originalNice; // original nice value of a thread
	int oldutime, oldstime, utime, stime; // aggregated execution time in jiffies
	int execTime; // execution time in last period in jiffies, equals utime + stime - oldutime - oldstime
	int coreId; // this thread can run on which core
	char name[18]; // thread name
} THREADATTR;

void initialize_threads(void);
void initialize_thread(int pid);
void destroy_thread(int pid);
void calculate_utilization(void);
float execTimeToUtil(int execTime);
void allocation(int pid);
void prioritize(vector *importanceChangeThrVec);
#endif // __THREAD_H__
