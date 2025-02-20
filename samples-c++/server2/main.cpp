//
// Created by shuxin ¡¡¡¡zheng on 2025/2/17.
//
#include <getopt.h>
#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "net_event/net_event.hpp"
#include "net_event/event_proc.hpp"
#include "net_event/client_socket.hpp"
#include "net_event/server_socket.hpp"

using namespace nev;

static void handle_client(client_socket *cli, int timeout) {
	(*cli).on_read([cli, timeout](socket_t fd, bool expired) {
		if (expired) {
			printf("Read timeout for fd %d\r\n", fd);
			cli->close();
			return;
		}

		char buf[1204];
		ssize_t ret = ::read(fd, buf, sizeof(buf));
		if (ret <= 0 || cli->write( buf, ret, timeout) == -1) {
			cli->close();
		}
	}).on_error([cli](socket_t fd) {
		cli->close();
	}).on_close([cli](socket_t fd) {
		delete cli;
	}).read_await(timeout);
}

class server_proc : public event_proc {
public:
	server_proc(net_event &ev, server_socket &ss, int timeout)
	: event_proc(ev, ss.sock_handle()), ss_(ss), timeout_(timeout) {}

	~server_proc() override  = default;

	bool stopped() const { return stop_; }

protected:
	// @override
	void on_read() override {
		std::string addr;
		int fd = ss_.accept(&addr);
		if (fd == -1) {
			printf("Accept error %s\r\n", strerror(errno));
			stop_ = true;
			return;
		}

		printf("Accept on client from %s, fd: %d\r\n", addr.c_str(), fd);

		auto *cli = new client_socket(this->get_event(), fd);
		handle_client(cli, timeout_);
	}

private:
	bool stop_ = false;
	server_socket &ss_;
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
	net_event_t etype = NET_EVENT_T_KERNEL;

	while ((ch = getopt(argc, argv, "he:l:p:t:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'e':
			if (strcmp(optarg, "poll") == 0) {
				etype = NET_EVENT_T_POLL;
				printf("Use pool\r\n");
			} else if (strcmp(optarg, "select") == 0) {
				etype = NET_EVENT_T_SELECT;
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

	server_socket ss(1024);
	if (!ss.open(ip.c_str(), port)) {
		printf("Listen on %s:%d error\r\n", ip.c_str(), port);
		return 1;
	}

	printf("Listen on %s:%d ok\r\n", ip.c_str(), port);

	net_event::debug(true);
	net_event ev(102400, etype);

	server_proc proc(ev, ss, timeout);
	proc.read_await();

	while (!proc.stopped()) {
		ev.wait(1000);
	}

	printf(">>>>Server stopped!<<<<\r\n");
	return 0;
}
