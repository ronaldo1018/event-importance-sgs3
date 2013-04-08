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
 * @file touch.h
 * @brief header file that control screen touch
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130317
 * @date 2013-03-17
 */
#ifndef __TOUCH_H__
#define __TOUCH_H__
#define TOUCH_INTERRUPT_PATH "/proc/interrupts"
#define TOUCH_REENABLE_PATH "/proc/mms_ts_touch_happen"
#include <stdbool.h>

void initialize_touch(void);
bool check_screen_touch(void);
void reenable_touch(void);
#endif // __TOUCH_H__
