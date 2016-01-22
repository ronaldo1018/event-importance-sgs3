#include "mytimerfd.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <unistd.h>
#include <sys/syscall.h>
#pragma GCC diagnostic pop

int my_timerfd_create(int clockid, int flags)
{
    return syscall(__NR_timerfd_create, clockid, flags);
}

int my_timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value)
{
    return syscall(__NR_timerfd_settime, fd, flags, new_value, old_value);
}

