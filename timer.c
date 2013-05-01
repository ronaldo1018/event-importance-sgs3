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
 * @file timer.c
 * @brief control timers
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#include "timer.h"
#include "config.h"
#include "importance.h"
#include "debug.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

extern bool agingIsOn;

/**
 * @brief initialize_timer initialize timers for utilization sampling and aging
 */
void initialize_timer(void)
{
	FILE *fp;
	INFO(("initialize timer\n"));

	// initialize first timer for utilization sampling
	fp = fopen(TIMER0_TIME_PATH, "w");
	if(fp)
	{
		fprintf(fp, "%d\n", CONFIG_UTILIZATION_SAMPLING_TIME);
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot setup timer0\n");
		destruction();
		exit(EXIT_FAILURE);
	}
	fp = fopen(TIMER0_ENABLE_PATH, "w");
	if(fp)
	{
		fprintf(fp, "1\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot setup timer0\n");
		destruction();
		exit(EXIT_FAILURE);
	}

	if(!CONFIG_TURN_ON_AGING)
		return;
	agingIsOn = true;
	// initialize second timer for aging
	fp = fopen(TIMER1_TIME_PATH, "w");
	if(fp)
	{
		fprintf(fp, "%d\n", CONFIG_INITIAL_AGING_TIME);
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot setup timer1\n");
		destruction();
		exit(EXIT_FAILURE);
	}
	fp = fopen(TIMER1_ENABLE_PATH, "w");
	if(fp)
	{
		fprintf(fp, "1\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot setup timer1\n");
		destruction();
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief destroy_timer stop all timers
 */
void destroy_timer(void)
{
	FILE *fp;
	INFO(("destroy timer\n"));

	fp = fopen(TIMER0_ENABLE_PATH, "w");
	if(fp)
	{
		fprintf(fp, "0\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot stop timer0\n");
		perror("timer0");
	}

	fp = fopen(TIMER1_ENABLE_PATH, "w");
	if(fp)
	{
		fprintf(fp, "0\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot stop timer1\n");
		perror("timer1");
	}

	fp = fopen(TIMER2_ENABLE_PATH, "w");
	if(fp)
	{
		fprintf(fp, "0\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot stop timer2\n");
		perror("timer2");
	}

	fp = fopen(TIMER3_ENABLE_PATH, "w");
	if(fp)
	{
		fprintf(fp, "0\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot stop timer3\n");
		perror("timer3");
	}
}

void turn_on_sampling_timer(void)
{
	FILE *fp;
	INFO(("turn on sampling timer\n"));

	fp = fopen(TIMER0_ENABLE_PATH, "w");
	if(fp)
	{
		fprintf(fp, "1\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot setup timer0\n");
		destruction();
		exit(EXIT_FAILURE);
	}
}

void turn_on_aging_timer(void)
{
	FILE *fp;
	INFO(("turn on aging timer\n"));

	fp = fopen(TIMER1_ENABLE_PATH, "w");
	if(fp)
	{
		fprintf(fp, "1\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot setup timer1\n");
		destruction();
		exit(EXIT_FAILURE);
	}
}

void turn_off_aging_timer(void)
{
	FILE *fp;
	INFO(("turn off aging timer\n"));

	fp = fopen(TIMER1_ENABLE_PATH, "w");
	if(fp)
	{
		fprintf(fp, "0\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot setup timer1\n");
		destruction();
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief turn_on_temp_mid_timer turn on temp mid timer for newly appeared thread that temporarily becomes mid
 */
void turn_on_temp_mid_timer(void)
{
	FILE *fp;
	INFO(("turn on temp mid timer\n"));

	fp = fopen(TIMER2_TIME_PATH, "w");
	if(fp)
	{
		fprintf(fp, "%d\n", CONFIG_TMP_MID_TIME);
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot setup timer2\n");
		destruction();
		exit(EXIT_FAILURE);
	}
	fp = fopen(TIMER2_ENABLE_PATH, "w");
	if(fp)
	{
		fprintf(fp, "1\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot setup timer2\n");
		destruction();
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief turn_on_temp_high_timer turn on temp high timer for foreground threads that temporarily become high
 */
void turn_on_temp_high_timer(void)
{
	FILE *fp;
	INFO(("turn on temp high timer\n"));

	// initialize first timer for utilization sampling
	fp = fopen(TIMER3_TIME_PATH, "w");
	if(fp)
	{
		fprintf(fp, "%d\n", CONFIG_TMP_HIGH_TIME);
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot setup timer3\n");
		destruction();
		exit(EXIT_FAILURE);
	}
	fp = fopen(TIMER3_ENABLE_PATH, "w");
	if(fp)
	{
		fprintf(fp, "1\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot setup timer3\n");
		destruction();
		exit(EXIT_FAILURE);
	}
}
