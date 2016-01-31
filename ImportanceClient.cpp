#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h> // sleep()
#include <binder/IBinder.h>
#include <binder/Binder.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include "config.h"
#include "debug.h"
#include "IImportance.h"
#include "ImportanceClient.h"
#include "importance_data.h"

#undef LOG_TAG
#define LOG_TAG "ImportanceService"

using namespace android;

SharedData* shm = NULL;

// Connect to server
int connect_service()
{
    if(!CONFIG_USE_BINDER)
    {
        return 0;
    }
    INFO(("connect to importance service\n"));
    sp<IImportance> service;
    while (true)
    {
        sp<IBinder> binder = defaultServiceManager()->getService(String16("importance"));
        service = interface_cast<IImportance>(binder);
        if (service.get())
        {
            INFO(("Importance service connected at %p", service.get()));
            break;
        }
        ERR(("Failed to connect to importance service! Retrying..."));
        sleep(1);
    }
    int fd = service->getFD();

    shm = reinterpret_cast<SharedData*>(mmap(NULL, 1024, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0));
    if (shm == MAP_FAILED)
    {
        ERR(("mmap of fd %d failed! %d, %s\n", fd, errno, strerror(errno)));
        return -1;
    }
    INFO(("Memory mapped at 0x%16llX\n", reinterpret_cast<unsigned long long>(shm)));
    return 0;
}

SharedData* getSharedData()
{
    return shm;
}
