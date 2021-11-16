#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "event.h"

static socket_t listen_addr(const char *ip, int port) {
	struct sockaddr_in sa;

	memset(&sa, 0, sizeof(sa));
	sa.sin_family      = PF_INET;
	sa.sin_port        = htons(port);
	sa.sin_addr.s_addr = inet_addr(ip);

	int lfd = socket(PF_INET, SOCK_STREAM, 0);
	int on = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if (bind(lfd, (const struct sockaddr*) &sa, sizeof(sa)) < 0) {
		close(lfd);
		return -1;
	}
	if (listen(lfd, 128) < 0) {
		close(lfd);
		return -1;
	}
	return lfd;
}

static void read_callback(EVENT *ev, FILE_EVENT *fe) {
	char buf[1024];
	int ret = read(fe->fd, buf, sizeof(buf));
	if (ret <= 0) {
		event_close(ev, fe);
		close(fe->fd);
		file_event_free(fe);
	} else if (write(fe->fd, buf, ret) <= 0) {
		event_close(ev, fe);
		close(fe->fd);
		file_event_free(fe);
	}
}

static void listen_callback(EVENT *ev, FILE_EVENT *fe) {
	struct sockaddr_in sa;
	socklen_t len = (socklen_t) sizeof(sa);
	memset(&sa, 0, sizeof(sa));
	socket_t cfd = accept(fe->fd, (struct sockaddr*) &sa, &len);
	if (cfd == -1) {
		printf("accept error %s\r\n", strerror(errno));
	} else {
		fe = file_event_alloc(cfd);
		event_add_read(ev, fe, read_callback);
	}
}

static void usage(const char *procname) {
	printf("usage: %s -s listen_ip -p listen_port\r\n", procname);
}

int main(int argc, char *argv[]) {
	int ch, port = 8088;
	char addr[64];

	snprintf(addr, sizeof(addr), "127.0.0.1");

	while ((ch = getopt(argc, argv, "hs:p:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		default:
			break;
		}
	}

	socket_t lfd = listen_addr(addr, port);
	if (lfd == -1) {
		printf("listen %s:%d error %s\r\n", addr, port, strerror(errno));
		return 1;
	}

	printf("listen on %s:%d\r\n", addr, port);

	EVENT *ev = event_create(10240, EVENT_EVENT_KERNEL);
	assert(ev);

	FILE_EVENT *fe = file_event_alloc(lfd);
	event_add_read(ev, fe, listen_callback);

	while (1) {
		event_process(ev, 1);
	}
	return 0;
}
