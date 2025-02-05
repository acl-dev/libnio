#ifndef EVENT_POLL_INCLUDE_H
#define EVENT_POLL_INCLUDE_H

#include "net_event.h"

#ifdef HAS_POLL

NET_EVENT *net_poll_create(int setsize);

#endif

#endif
