//
// Created by shuxin ����zheng on 2025/2/17.
//
#include <getopt.h>
#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "nio/nio_event.hpp"
#include "nio/event_proc.hpp"
#include "nio/client_socket.hpp"
#include "nio/server_socket.hpp"

using namespace nio;

static void handle_client(client_socket *cli, int timeout) {
	(*cli).on_read([cli, timeout](socket_t fd, bool expired) {
		if (expired) {
			printf("Read timeout for fd %d\r\n", fd);
			cli->close_await();
			return;
		}

		char buf[1204];
		ssize_t ret = ::read(fd, buf, sizeof(buf));
		if (ret <= 0 || cli->write(buf, ret, timeout) == -1) {
			cli->close_await();
		}
	}).on_write([cli](socket_t fd, bool expired) {
		if (expired) {
			printf("Write expired for fd %d\r\n", fd);
			cli->close_await();
		}
	}).on_error([cli](socket_t fd) {
		cli->close_await();
	}).on_close([cli](socket_t fd) {
        printf("Closing client fd %d\r\n", fd);
		delete cli;
	}).read_await(timeout);
}

class server_proc : public server_socket {
public:
	server_proc(nio_event &ev, int timeout)
	: server_socket(ev), timeout_(timeout) {}

	~server_proc() override  = default;

	bool stopped() const { return stop_; }

protected:
	// @override
	void on_accept(socket_t fd, const std::string &addr) override {
		printf("Accept on client from %s, fd: %d\r\n", addr.c_str(), fd);

		auto *cli = new client_socket(*this->get_event(), fd);
		handle_client(cli, timeout_);
	}

private:
	bool stop_ = false;
	int timeout_;
};

static void usage(const char *procname) {
	printf("Usage: %s -h[help]\r\n"
		   " -e event_type[kernel|poll|select, default: kernel]\r\n"
		   " -l listen_ip[default: 127.0.0.1]\r\n"
		   " -p port[default: 8288]\r\n"
		   " -t IO timeout in ms[default: 5000]\r\n", procname);
}

int main(int argc, char *argv[]) {
	int ch;
	std::string ip("127.0.0.1");
	int port = 8288, timeout = 5000;
	nio_event_t etype = NIO_EVENT_T_KERNEL;

	while ((ch = getopt(argc, argv, "he:l:p:t:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'e':
			if (strcmp(optarg, "poll") == 0) {
				etype = NIO_EVENT_T_POLL;
				printf("Use pool\r\n");
			} else if (strcmp(optarg, "select") == 0) {
				etype = NIO_EVENT_T_SELECT;
				printf("Use select\r\n");
			} else if (strcmp(optarg, "kernel") != 0) {
				printf("Unknown event: %s\r\n", optarg);
				usage(argv[0]);
				return 1;
			} else {
				printf("Use kernel\r\n");
			}
			break;
		case 'l':
			ip = optarg;
			break;
		case 'p':
			port = std::atoi(optarg);
			break;
		case 't':
			timeout = std::atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	signal(SIGPIPE, SIG_IGN);

	nio_event::debug(true);
	nio_event ev(102400, etype);

	server_proc server(ev, timeout);
	if (!server.open(ip.c_str(), port)) {
		printf("Listen on %s:%d error\r\n", ip.c_str(), port);
		return 1;
	}
	printf("Listen on %s:%d ok\r\n", ip.c_str(), port);

	server.accept_await();

	while (!server.stopped()) {
		ev.wait(1000);
	}

	printf(">>>>Server stopped!<<<<\r\n");
	return 0;
}
