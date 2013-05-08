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
 * @file debug.h
 * @brief header file that define debug functions
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#ifndef __DEBUG_H__
#define __DEBUG_H__
#define DEBUG_ERR 0
#define DEBUG_INFO 0
#define DEBUG_DVFS_INFO 0
#define ERR(x) do { if (DEBUG_ERR) dbg_printf x; } while (0)
#define INFO(x) do { if (DEBUG_INFO) dbg_printf x; } while (0)
#define DVFS_INFO(x) do { if (DEBUG_DVFS_INFO) dbg_printf x; } while (0)
void initialize_debug(void);
void dbg_printf(const char *fmt, ...);

#endif // __DEBUG_H__
