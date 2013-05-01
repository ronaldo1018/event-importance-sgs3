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
 * @file activity.h
 * @brief header file that control activity
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#ifndef __ACTIVITY_H__
#define __ACTIVITY_H__
#define ACTIVITY_CMD "dumpsys activity top"
#include "vector.h"
#include <stdbool.h>

void initialize_activity(void);
bool check_activity_change(vector *becomeBGThrVec, vector *becomeFGThrVec);
void getCurActivityThrs(vector *curActivityThrVec);
#endif // __ACTIVITY_H__
