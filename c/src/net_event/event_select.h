#ifndef EVENT_SELECT_INCLUDE_H
#define EVENT_SELECT_INCLUDE_H

#include "net_event.h"

#ifdef HAS_SELECT

NET_EVENT *net_select_create(int setsize);

#endif

#endif
