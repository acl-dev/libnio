#ifndef EVENT_EPOLL_INCLUDE_H
#define EVENT_EPOLL_INCLUDE_H

#include "event.h"

#ifdef HAS_EPOLL

NET_EVENT *net_epoll_create(int setsize);

#endif

#endif
