#include <unistd.h>

int my_get_nprocs_conf()
{
    static int result = 0;
    if (result > 0)
    {
        return result;
    }
    result = sysconf(_SC_NPROCESSORS_CONF);
    return result;
}
