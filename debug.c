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
 * @file debug.c
 * @brief debug functions
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#include "debug.h"

#define USE_LOGCAT 0

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h> // gettimeofday()
#if USE_LOGCAT
#include <android/log.h>
#endif
#pragma GCC diagnostic pop

static struct timeval startTime;
static struct timeval curTime;
static struct timezone timez;

/**
 * @brief initialize_debug initialize startTime to know how long this process has run
 */
void initialize_debug(void)
{
	gettimeofday(&startTime, &timez);
}

/**
 * @brief dbg_printf debug printing function
 *
 * @param fmt
 * @param ...
 */
void dbg_printf(const char *fmt, ...)
{
    va_list args;

	gettimeofday(&curTime, &timez);
	fprintf(stderr, "%f: ", curTime.tv_sec - startTime.tv_sec + (float)(curTime.tv_usec - startTime.tv_usec) / 1000000.0);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
#if USE_LOGCAT
    __android_log_vprint (ANDROID_LOG_DEBUG, "importance", fmt, args);
#endif
    va_end(args);
}
