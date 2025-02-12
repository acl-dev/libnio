#ifndef EVENTS_IOCP_INCLUDE_H
#define EVENTS_IOCP_INCLUDE_H

#include "net_event.h"

#ifdef HAS_IOCP

NET_EVENT *net_iocp_create(int setsize);

#endif

#endif
