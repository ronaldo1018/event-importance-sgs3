#ifndef _MYTIMERFD_H
#define _MYTIMERFD_H

#include <time.h>

#define TFD_TIMER_ABSTIME 1

int my_timerfd_create(int clockid, int flags);
int my_timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);

#endif

