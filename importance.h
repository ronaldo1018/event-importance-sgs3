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
 * @file importance.h
 * @brief header file that control thread importance
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#ifndef __IMPORTANCE_H__
#define __IMPORTANCE_H__
#include <stdbool.h>
enum IMPORTANCE_VALUE{IMPORTANCE_LOW = 1, IMPORTANCE_MID = 2, IMPORTANCE_HIGH = 3};

void change_importance(int pid, enum IMPORTANCE_VALUE importance, bool firstAssign);
void destruction(void);
#endif // __IMPORTANCE_H__
