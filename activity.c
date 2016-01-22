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
 * @file activity.c
 * @brief control activity
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#include "activity.h"
#include "thread.h"
#include "common.h"
#include "debug.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <stdio.h>
#include <stdlib.h> // atoi()
#include <ctype.h> // isdigit()
#include <string.h>
#include <dirent.h> // opendir(), readdir(), closedir()
#pragma GCC diagnostic pop

extern THREADATTR threadSet[];
extern vector *curActivityThrVec;

// variables or functions only used in this file
/**
 * @brief current foreground activity thread's pid
 */
static int curActivityPid = -1;

static int getCurActivityPid(void);
static void putThreadGroupToVector(vector *threadGroup, int pid);

/**
 * @brief initialize_activity get current activity to initialize curActivityPid
 */
void initialize_activity(void)
{
	unsigned int i, length;
	int pid;
	INFO(("initialize activity\n"));

	// initialize current activity and change importance of foreground thread to mid
	curActivityPid = getCurActivityPid();
	getCurActivityThrs(curActivityThrVec);
	length = vector_length(curActivityThrVec);
	for(i = 0; i < length; i++)
	{
		pid = ((int *)curActivityThrVec->elems)[i];
		change_importance(pid, IMPORTANCE_MID, false);
	}
}

/**
 * @brief check_activity_change profile current activity and notify the change
 *
 * @param becomeBGThrVec will store threads become background threads
 * @param becomeFGThrVec will store threads become foreground threads
 *
 * @return true if foreground activity change, false otherwise
 */
bool check_activity_change(vector *becomeBGThrVec, vector *becomeFGThrVec)
{
	int newPid;
	INFO(("check activity change\n"));

	newPid = getCurActivityPid();

	if(newPid != -1 && newPid != curActivityPid)
	{
		INFO(("activity change, old activity pid = %d, new activity pid = %d\n", curActivityPid, newPid));
		putThreadGroupToVector(becomeBGThrVec, curActivityPid);
		putThreadGroupToVector(becomeFGThrVec, newPid);
		curActivityPid = newPid;
		return true;
	}
	return false;
}

/*
 * retrieve current foreground activity threads into curActivityThrVec
 */
/**
 * @brief getCurActivityThrs retrieve current foreground threads into curActivityThrVec
 *
 * @param curActivityThrVec will store current foreground threads
 */
void getCurActivityThrs(vector *vec)
{
	INFO(("get current activity threads\n"));
	if(curActivityPid != -1)
	{
		vector_remove_all(vec);
		putThreadGroupToVector(vec, curActivityPid);
	}
}

/**
 * @brief getCurActivityPid use 'dumpsys activity top' to get current activity pid
 *
 * @return current activity pid, -1 on error
 */
static int getCurActivityPid(void)
{
	FILE *fp;
	char buff[BUFF_SIZE] = "";
	char tmp[BUFF_SIZE] = "";
	int pid = -1;
	bool found = false;

	fp = popen(ACTIVITY_CMD, "r");
	if(fp)
	{
		while(fgets(buff, BUFF_SIZE, fp))
		{
			if(strstr(buff, "ACTIVITY"))
			{
				// note: dumpsys activity top may return (not running) in pid field (but rarely) so don't always trust parsing
				sscanf(buff, " ACTIVITY %s %s pid=%d", tmp, tmp, &pid);
				found = true;
				break;
			}
		}
		pclose(fp);
	}
	else
	{
		ERR(("cannot popen dumpsys\n"));
	}

	if(found)
	{
		INFO(("current activity pid = %d\n", pid));
		return pid;
	}
	else
	{
		ERR(("cannot get activity pid\n"));
		return -1;
	}

	return pid;
}

/**
 * @brief putThreadGroupToVector put threads' pid in the same task group(from /proc/[pid]/task) to vector
 *
 * @param threadGroup
 * @param pid
 */
static void putThreadGroupToVector(vector *threadGroup, int pid)
{
	char buff[BUFF_SIZE];
	char tmp[1024] = "";
	DIR *d;
	struct dirent *de;
	INFO(("put thread group of %d into vector\n", pid));

	if(pid == -1)
		return;
	
	sprintf(buff, "/proc/%d/task", pid);
	if((d = opendir(buff)) != NULL)
	{
		strcat(tmp, "Threads ");
		while((de = readdir(d)) != 0)
		{
			if(isdigit(de->d_name[0]))
			{
				pid = atoi(de->d_name);
				sprintf(buff, "%d ", pid);
				strcat(tmp, buff);
				vector_push(threadGroup, &pid);
			}
		}
		closedir(d);
		strcat(tmp, "are pushed to the same group\n");
		INFO(("%s\n", tmp));
	}
	else
	{
		ERR(("directory %s not found\n", buff));
	}
}
