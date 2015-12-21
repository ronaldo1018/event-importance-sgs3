#ifndef _DEVICE_EVENTS_H
#define _DEVICE_EVENTS_H

#include "importance_data.h"

void initialize_fifo();
void handle_device_events(struct FIFO_DATA fifo_data);
int get_fifo_fd();

#endif
