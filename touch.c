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
#include "debug.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#pragma GCC diagnostic pop
#include "ImportanceClient.h"

/*
 * get current screen touch counts to initialize lastTouchCount
 */
void initialize_touch(void)
{
	INFO(("initialize touch\n"));

    reenable_touch();
}

/**
 * @brief reenable_touch reenable touch connector to listen event
 */
void reenable_touch(void)
{
    INFO(("reenable_touch\n"));
    struct SharedData *shm = getSharedData();
    if (shm)
    {
        getSharedData()->touch_enabled = 1;
    }
    else
    {
        INFO(("reenable_touch(): SHM not ready yet.\n"));
    }
}
