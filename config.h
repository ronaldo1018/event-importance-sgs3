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
 * @file config.h
 * @brief config file
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CONFIG_TURN_ON_ALLOCATION 1
#define CONFIG_TURN_ON_DPM 0
#define CONFIG_TURN_ON_MIGRATION 1
#define CONFIG_TURN_ON_PRIORITIZE 1
#define CONFIG_TURN_ON_DVFS 1
#define CONFIG_TURN_ON_AGING 1

#define CONFIG_USE_FIFO 1
#define CONFIG_USE_BINDER 1
#define CONFIG_USE_CONNECTOR 1

#define CONFIG_FAIL_WHEN_OPEN_CORES_FAIL 0

// timer
#define CONFIG_CORE_UPDATE_TIME 105
#define CONFIG_HZ 200
#define JIFFIES_TIME(x)             ((int)(CONFIG_HZ * x))
#define CONFIG_UTILIZATION_SAMPLING_TIME	0.2
#define CONFIG_TMP_HIGH_TIME				0.5
#define CONFIG_CHECK_ACTIVITY_TIME			5
// nice
#define CONFIG_NICE_HIGH					-20
#define CONFIG_NICE_MID						-10
#define CONFIG_NICE_LOW						10
#define CONFIG_MID_WEIGHT_OVER_LOW			86.74f /* (1.25^20) */
// DVFS
#define CONFIG_USE_TIME_IN_STATE_FREQUENCY_TABLE 1
#define CONFIG_SCALE_UP_RATIO 0.75f
#define CONFIG_NUM_OF_HISTORY_ENTRIES 2
#define CONFIG_USE_CURRENT_FREQUENCY_AS_MAXIMUM 0
#define CONFIG_MINIMUM_FREQ 200000
// DPM
#define CONFIG_THRESHOLD2 400000 /* switch threshold of 1 or 2 core, utilsum = 400000 */
#define CONFIG_THRESHOLD3 800000 /* switch threshold of 2 or 3 core, utilsum = 800000 */
#define CONFIG_THRESHOLD4 1700000 /* switch threshold of 3 or 4 core, utilsum = 1700000 */
#define CONFIG_NUM_OF_PROCESS_RUNNING_HISTORY_ENTRIES 3

#define CONFIG_MEASURE_OVERHEAD 0
#endif // __CONFIG_H__
