#ifndef EVENT_POLL_INCLUDE_H
#define EVENT_POLL_INCLUDE_H

#include "event.h"

#ifdef HAS_POLL

NIO_EVENT *nio_poll_create(int setsize);

#endif

#endif
