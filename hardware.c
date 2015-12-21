#include <sys/epoll.h>

extern int epollfd;

int register_device_events ()
{
    struct epoll_event epollev;

    epollev.data.fd = fd;
    epollev.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &epollev);
}
