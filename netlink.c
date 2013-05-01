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
 * @file netlink.c
 * @brief control netlink socket
 * @author Po-Hsien Tseng <steve13814@gmail.com>
 * @version 20130409
 * @date 2013-04-09
 */
#include "netlink.h" // already include netlink.h, connector.h, cn_proc.h
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

/**
 * @brief get_netlink_socket connect to netlink
 *
 * @return netlink socket, -1 on error
 */
int get_netlink_socket()
{
    int nlSock;
    struct sockaddr_nl sa_nl;

	INFO(("create netlink socket\n"));
    if((nlSock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR)) == -1)
	{
		fprintf(stderr, "cannot create netlink socket\n");
        return -1;
    }

    sa_nl.nl_family = AF_NETLINK;
    sa_nl.nl_groups = CN_IDX_PROC;
    sa_nl.nl_pid = getpid();

	INFO(("bind netlink socket\n"));
    if(bind(nlSock, (struct sockaddr *)&sa_nl, sizeof(sa_nl)) == -1)
	{
		fprintf(stderr, "cannot bind netlink socket\n");
        close(nlSock);
        return -1;
    }
	return nlSock;
}

/**
 * @brief set_proc_event_listen subscribe proc events if enable is true, unsubscribe proc events if enable is false
 *
 * @param nlSock
 * @param enable
 *
 * @return return 0 on success, -1 otherwise
 */
int set_proc_event_listen(int nlSock, bool enable)
{
    NLCN_MSG nlcnMsg;

    memset(&nlcnMsg, 0, sizeof(nlcnMsg));
    nlcnMsg.nl_hdr.nlmsg_len = sizeof(nlcnMsg);
    nlcnMsg.nl_hdr.nlmsg_pid = getpid();
    nlcnMsg.nl_hdr.nlmsg_type = NLMSG_DONE;

    nlcnMsg.cn_msg.id.idx = CN_IDX_PROC;
    nlcnMsg.cn_msg.id.val = CN_VAL_PROC;
    nlcnMsg.cn_msg.len = sizeof(enum proc_cn_mcast_op);

    nlcnMsg.cn_mcast = enable ? PROC_CN_MCAST_LISTEN : PROC_CN_MCAST_IGNORE;

	INFO(("send message, %s\n", enable ? "subscribe proc connector" : "unsubscribe proc connector"));
    if(send(nlSock, &nlcnMsg, sizeof(nlcnMsg), 0) == -1)
	{
		fprintf(stderr, "cannot send netlink message\n");
        return -1;
    }

    return 0;
}
