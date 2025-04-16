#ifndef EVENT_WMSG_INCLUDE_H
#define EVENT_WMSG_INCLUDE_H

#include "event.h"

#ifdef HAS_WMSG

NIO_EVENT *nio_wmsg_create(int setsize);

#endif

#endif
