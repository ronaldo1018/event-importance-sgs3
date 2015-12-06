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
/*
 * notice: timers trigger repeatedly, need to stop manually
 */

#define N_TIMERS 2

#define TIMER_UTILIZATION_SAMPLING      0
#define TIMER_TMP_HIGH                  1

void initialize_timer(void);
void destroy_timer(void);
void turn_on_timer(int timerid);
void turn_off_timer(int timerid);

#endif // __TIMER_H__
