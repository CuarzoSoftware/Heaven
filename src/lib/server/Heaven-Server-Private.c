#include "Heaven-Server-Private.h"
#include "Heaven-Server.h"

int hv_fds_add_fd(struct pollfd *fds, int fd)
{
    for(int i = 2; i < HV_MAX_CLIENTS+2; i++)
    {
        if(fds[i].fd == -1)
        {
            fds[i].fd = fd;
            return i;
        }
    }

    return -1;
}

