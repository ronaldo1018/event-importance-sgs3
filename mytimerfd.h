#ifndef _MYTIMERFD_H
#define _MYTIMERFD_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <time.h>
#pragma GCC diagnostic pop

#define TFD_TIMER_ABSTIME 1

int my_timerfd_create(int clockid, int flags);
int my_timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);

#endif

