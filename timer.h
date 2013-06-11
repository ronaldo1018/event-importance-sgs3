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
 * @file timer.h
 * @brief header file that control timers
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#ifndef __TIMER_H__
#define __TIMER_H__
#include <stdbool.h>
#define TIMER0_ENABLE_PATH "/proc/timer_connector/timer0_enable" // util_sample
#define TIMER1_ENABLE_PATH "/proc/timer_connector/timer1_enable" // temp_high
#define TIMER0_TIME_PATH "/proc/timer_connector/timer0_time"
#define TIMER1_TIME_PATH "/proc/timer_connector/timer1_time"
/*
 * notice: all timers only turn on once, need manually restart
 */

void initialize_timer(void);
void destroy_timer(void);
void turn_on_sampling_timer(void);
void turn_on_temp_high_timer(void);

#endif // __TIMER_H__
