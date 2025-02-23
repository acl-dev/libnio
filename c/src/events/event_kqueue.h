#ifndef	EVENT_KQUEUE_INCLUDE_H
#define	EVENT_KQUEUE_INCLUDE_H

#include "event.h"

#ifdef	HAS_KQUEUE

NIO_EVENT *nio_kqueue_create(int setsize);

#endif

#endif
