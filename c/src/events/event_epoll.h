#ifndef EVENT_EPOLL_INCLUDE_H
#define EVENT_EPOLL_INCLUDE_H

#include "event.h"

#ifdef HAS_EPOLL

NIO_EVENT *nio_epoll_create(int setsize);

#endif

#endif
