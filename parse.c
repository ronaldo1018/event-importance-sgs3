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
 * @file parse.c
 * @brief parsing functions
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#include "parse.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <stdio.h>
#include <stdlib.h>
#pragma GCC diagnostic pop
#define BUFF_SIZE 256

/**
 * @brief parseString parse space seperated data, column start from zero
 *
 * @param path where to parse
 * @param target place where data will store
 * @param col get col-th data to target
 * @param failbit will set to 1 if cannot parse
 */
void parseString(char *path, int col, char *target, bool *failbit)
{
	int i = 0;
	char buff[BUFF_SIZE];
	FILE *fp = fopen(path, "r");
	if(fp)
	{
		while(i++ != col && fscanf(fp, "%s", buff));
		fscanf(fp, "%s", target);
		(*failbit) = 0;
		fclose(fp);
	}
	else
	{
		(*failbit) = 1;
	}
}

/**
 * @brief parseInt parse space seperated data, column and row number start from zero
 *
 * @param path
 * @param row
 * @param col
 * @param failbit
 *
 * @return an integer that are parsed, -1 on error
 */
int parseInt(char *path, int row, int col, bool *failbit)
{
	FILE *fp = fopen(path, "r");
	char buff[BUFF_SIZE];
	int target = -1;
	int i = 0, j = 0;
	if(fp)
	{
		while(i++ != row && fgets(buff, BUFF_SIZE, fp));
		while(j++ != col && fscanf(fp, "%s", buff));
		fscanf(fp, "%d", &target);
		(*failbit) = 0;
		fclose(fp);
	}
	else
	{
		(*failbit) = 1;
	}
	return target;
}

/**
 * @brief parseInt2 parse space seperated data, return two parsed integers to target1 and target2, column and row number start from zero
 *
 * @param path
 * @param col1
 * @param target1
 * @param col2
 * @param target2
 * @param failbit
 */
void parseInt2(char *path, int col1, int *target1, int col2, int *target2, bool *failbit)
{
	FILE *fp = fopen(path, "r");
	char buff[BUFF_SIZE];
	int i = 0;
	if(col1 > col2)
	{
		int tmp = col1;
		col1 = col2;
		col2 = tmp;
	}
	if(fp) 
	{
		while(i++ != col1 && fscanf(fp, "%s", buff));
		fscanf(fp, "%d", target1);
		while(i++ != col2 && fscanf(fp, "%s", buff));
		fscanf(fp, "%d", target2);
		(*failbit) = 0;
		fclose(fp);
	}
	else
	{
		(*failbit) = 1;
	}
}
