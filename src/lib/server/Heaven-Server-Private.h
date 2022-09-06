#ifndef HEAVENSERVERPRIVATE_H
#define HEAVENSERVERPRIVATE_H

#include "../common/Heaven-Common.h"
#include <poll.h>

// Returns insert index or -1 if no empty spaces
int hv_fds_add_fd(struct pollfd *fds, int fd);

#endif // HEAVENSERVERPRIVATE_H
