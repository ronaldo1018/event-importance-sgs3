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
 * @file netlink.h
 * @brief header file that control netlink socket
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#ifndef __NETLINK_H__
#define __NETLINK_H__
#ifndef __GNUC__
#define __attribute__(x) /*NOTHING*/
#endif
#include <stdbool.h>
#include <linux/netlink.h>
#include <connector.h> // from external include dir
#include <cn_proc.h> // from external include dir

// __attribute__ tells gnu compiler to do some checking or modification to variable
// aligned tells the minimal aligned size in bytes
// packed tells the aligned size is 1 byte(no spacing)
typedef struct __attribute__((__aligned__(NLMSG_ALIGNTO)))
{
	struct nlmsghdr nl_hdr;
	struct __attribute__((__packed__))
	{
		struct cn_msg cn_msg;
		enum proc_cn_mcast_op cn_mcast;
	};
} NLCN_MSG;

typedef struct __attribute__((aligned(NLMSG_ALIGNTO)))
{
	struct nlmsghdr nl_hdr;
	struct __attribute__((__packed__))
	{
		struct cn_msg cn_msg;
		struct proc_event proc_ev;
    };
} NLCN_CNPROC_MSG;

int get_netlink_socket();
int set_proc_event_listen(int nl_sock, bool enable);

#endif // __NETLINK_H__
