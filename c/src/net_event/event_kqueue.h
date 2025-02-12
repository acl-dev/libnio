#ifndef	EVENT_KQUEUE_INCLUDE_H
#define	EVENT_KQUEUE_INCLUDE_H

#include "net_event.h"

#ifdef	HAS_KQUEUE

NET_EVENT *net_kqueue_create(int setsize);

#endif

#endif
