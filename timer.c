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
#include <string.h>
#include <time.h>
#include <sys/epoll.h>
#include "mytimerfd.h"

extern bool agingIsOn;

extern int epollfd;

int timerfd[N_TIMERS];
static struct timespec timerspecs[N_TIMERS];

/**
 * @brief initialize_timer initialize timers for utilization sampling and aging
 */
void initialize_timer(void)
{
    int i;

	INFO(("initialize timer\n"));

    for (i = 0; i < N_TIMERS; i++)
    {
        timerfd[i] = my_timerfd_create(CLOCK_REALTIME, 0);
        timerspecs[i].tv_sec = 0;
    }

    timerspecs[0].tv_nsec = CONFIG_UTILIZATION_SAMPLING_TIME * 1E9;
    timerspecs[1].tv_nsec = CONFIG_TMP_HIGH_TIME * 1E9;

	if(CONFIG_TURN_ON_AGING)
		agingIsOn = true;
}

/**
 * @brief destroy_timer stop all timers
 */
void destroy_timer(void)
{
	INFO(("destroy timer\n"));
    /* Currently do nothing as timers stop as the program ends */
}

void turn_on_timer(int timerid)
{
    struct epoll_event epollev;
    struct timespec now;
    struct itimerspec timerspec;

    INFO(("turn on timer %d\n", timerid));

    clock_gettime(CLOCK_REALTIME, &now);

    now.tv_nsec += timerspecs[timerid].tv_nsec;
    now.tv_sec += timerspecs[timerid].tv_sec;
    if (now.tv_nsec >= 1E9)
    {
        now.tv_sec += 1;
        now.tv_nsec -= 1E9;
    }

    memcpy(&timerspec.it_value, &now, sizeof(struct timespec));
    memcpy(&timerspec.it_interval, &timerspecs[timerid], sizeof(struct timespec));
    my_timerfd_settime(timerfd[timerid], TFD_TIMER_ABSTIME, &timerspec, NULL);

    epollev.data.fd = timerfd[timerid];
    epollev.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, timerfd[timerid], &epollev);
}

void turn_off_timer(int timerid)
{
    struct itimerspec timerspec;

    INFO(("turn off timer %d\n", timerid));

    memset(&timerspec.it_value, 0, sizeof(struct timespec));

    my_timerfd_settime(timerfd[timerid], TFD_TIMER_ABSTIME, &timerspec, NULL);

    epoll_ctl(epollfd, EPOLL_CTL_DEL, timerfd[timerid], NULL);
}
