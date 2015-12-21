#include <sys/mman.h>
#include <pthread.h>
#include <binder/IBinder.h>
#include <binder/Binder.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include "debug.h"
#include "IImportance.h"
#include "ImportanceClient.h"
#include "importance_data.h"

#undef LOG_TAG
#define LOG_TAG "ImportanceService"

using namespace android;

SharedData* shm;

// Connect to server
int connect_service()
{
    sp<IBinder> binder = defaultServiceManager()->getService(String16("importance"));
    sp<IImportance> service = interface_cast<IImportance>(binder);
    if (!service.get())
    {
        ERR(("Failed to connect to importance service!"));
        return -2;
    }
    int fd = service->getFD();

    shm = reinterpret_cast<SharedData*>(mmap(NULL, 1024, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0));
    if (shm == MAP_FAILED)
    {
        ERR(("mmap of fd %d failed! %d, %s\n", fd, errno, strerror(errno)));
        return -1;
    }
    INFO(("Memory mapped at 0x%16X\n", shm));
    return 0;
}

extern "C" SharedData* getSharedData()
{
    return shm;
}
