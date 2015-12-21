#ifndef _ImportanceClient_h
#define _ImportanceClient_h

#include "importance_data.h"

#ifdef __cplusplus
extern "C" {
#endif

int connect_service();
struct SharedData* getSharedData();

#ifdef __cplusplus
}
#endif

#endif
