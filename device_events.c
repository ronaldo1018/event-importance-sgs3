#include "device_events.h"
#include "importance.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <fcntl.h>
#pragma GCC diagnostic pop

static int fifo_fd = -1;

void initialize_fifo()
{
    struct epoll_event epollev;

    fifo_fd = open ("/data/local/tmp/importance_fifo", O_RDONLY|O_NONBLOCK);
    if (fifo_fd < 0)
    {
        exit (EXIT_FAILURE);
    }
    epollev.data.fd = fifo_fd;
    epollev.events = EPOLLIN;
    epoll_ctl(get_epoll_fd(), EPOLL_CTL_ADD, fifo_fd, &epollev);
}

void handle_device_events(struct FIFO_DATA fifo_data)
{
    switch (fifo_data.type)
    {
        case SCREEN_EVENT_TYPE:
            handle_screen_onoff(fifo_data.data.screen_on);
            break;
        case TOUCH_EVENT_TYPE:
            handle_touch();
            break;
    }
}

int get_fifo_fd()
{
    return fifo_fd;
}
