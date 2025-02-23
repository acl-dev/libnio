#ifndef EVENTS_IOCP_INCLUDE_H
#define EVENTS_IOCP_INCLUDE_H

#include "event.h"

#ifdef HAS_IOCP

NIO_EVENT *nio_iocp_create(int setsize);

#endif

#endif
