//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#pragma once

#include <cassert>
#include <sys/time.h>
#include <string.h>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "../../c/include/net_event/net_event.h"

typedef union {
	struct sockaddr_storage ss;
#ifdef AF_INET6
	struct sockaddr_in6 in6;
#endif
	struct sockaddr_in in;
#ifdef ACL_UNIX
	struct sockaddr_un un;
#endif
	struct sockaddr sa;
} SOCK_ADDR;
