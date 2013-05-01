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
 * @file touch.c
 * @brief control screen touch
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#include "touch.h"
#include "config.h"
#include "common.h"
#include <stdio.h>
#include <string.h>

static unsigned int lastTouchCount = 0;

/*
 * get current screen touch counts to initialize lastTouchCount
 */
void initialize_touch(void)
{
	char buff[BUFF_SIZE];
	char tmp[BUFF_SIZE];
	if(CONFIG_POLLING_TOUCH_STATUS)
	{
		FILE *fp = fopen(TOUCH_INTERRUPT_PATH, "r");
		if(fp)
		{
			while(fgets(buff, BUFF_SIZE, fp))
			{
				if(strstr(buff, CONFIG_TOUCH_INTERRUPT_DRIVER))
				{
					sscanf(buff, "%s %d %s", tmp, &lastTouchCount, tmp);
					break;
				}
			}
			fclose(fp);
		}
	}
	else // event driven touch need to make sure touch happen entry is clear so manual clear it
	{
		reenable_touch();
	}
}

/*
 * profile current screen touch count and notify the change
 * return true if touch count increase, false otherwise
 */
bool check_screen_touch(void)
{
	char buff[BUFF_SIZE];
	char tmp[BUFF_SIZE];
	unsigned int touchCount;
	bool check = false;
	FILE *fp = fopen(TOUCH_INTERRUPT_PATH, "r");
	if(fp)
	{
		while(fgets(buff, BUFF_SIZE, fp))
		{
			if(strstr(buff, CONFIG_TOUCH_INTERRUPT_DRIVER))
			{
				sscanf(buff, "%s %u %s", tmp, &touchCount, tmp);
				if(touchCount > lastTouchCount)
				{
					lastTouchCount = touchCount;
					check = true;
				}
				break;
			}
		}
		fclose(fp);
	}
	return check;
}

/**
 * @brief reenable_touch reenable touch connector to listen event
 */
void reenable_touch(void)
{
	FILE *fp = fopen(TOUCH_REENABLE_PATH, "w");
	if(fp)
	{
		fprintf(fp, "0\n");
		fclose(fp);
	}
}
