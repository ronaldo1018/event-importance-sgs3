#include "mytimerfd.h"
#include <unistd.h>
#include <sys/syscall.h>

int my_timerfd_create(int clockid, int flags)
{
    return syscall(__NR_timerfd_create, clockid, flags);
}

int my_timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value)
{
    return syscall(__NR_timerfd_settime, fd, flags, new_value, old_value);
}

