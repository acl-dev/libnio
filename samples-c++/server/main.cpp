//
// Created by shuxin ¡¡¡¡zheng on 2025/2/17.
//
#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "net_event/net_event.hpp"
#include "net_event/event_proc.hpp"
#include "net_event/event_timer.hpp"
#include "net_event/server_socket.hpp"

using namespace ev;

class client_proc : public event_proc, public event_timer {
public:
	client_proc(net_event &ev, int fd, int timeout)
	: event_proc(ev, fd), fd_(fd)
	, timeout_(timeout)
	{
		this->get_event().add_timer(this, timeout);
	}

	~client_proc() override {
		close(fd_);
		printf("Close fd %d\r\n", fd_);
	}

protected:
	// @override from event_proc
	bool read() override {
		char buf[4096];
		ssize_t ret = ::read(fd_, buf, sizeof(buf));
		if (ret <= 0) {
			this->get_event().del_timer(this);
			this->get_event().delay_destroy(this);
			return false;
		}

		if (::write(fd_, buf, (size_t) ret) != ret) {
			this->get_event().del_timer(this);
			this->get_event().delay_destroy(this);
			return false;
		}

		this->get_event().reset_timer(this, timeout_);
		return true;
	}

	// @override from event_timer
	void on_timer() override {
		printf("Read timeout from fd: %d\r\n", fd_);
		const char *s = "Timeout, bye!\r\n";
		::write(fd_, s, strlen(s));
		this->disable_read();
		this->get_event().delay_destroy(this);
	}

	// @override from event_proc
	void destroy() override {
		delete this;
	}

private:
	int fd_;
	int timeout_;
};

class server_proc : public event_proc {
public:
	server_proc(net_event &ev, server_socket &ss, int timeout)
	: event_proc(ev, ss.sock_handle()), ss_(ss), timeout_(timeout) {}

	~server_proc() override = default;

	bool stopped() const { return stop_; }

protected:
	// @override
	bool read() override {
		std::string addr;
		int fd = ss_.accept(&addr);
		if (fd == -1) {
			stop_ = true;
			return false;
		}

		printf("Accept on client from %s, fd: %d\r\n", addr.c_str(), fd);
		event_proc *proc = new client_proc(this->get_event(), fd, timeout_);
		proc->read_await();
		return true;
	}

private:
	bool stop_ = false;
	server_socket &ss_;
	int timeout_;
};

static void usage(const char *procname) {
	printf("Usage: %s -h[help]\r\n"
		   " -l listen_ip[default: 127.0.0.1]\r\n"
		   " -p port[default: 8288]\r\n"
		   " -t IO timeout in ms[default: 5000]\r\n", procname);
}

int main(int argc, char *argv[]) {
	int ch;
	std::string ip("127.0.0.1");
	int port = 8288, timeout = 5000;

	while ((ch = getopt(argc, argv, "hl:p:t:")) > 0) {
		switch (ch) {
			case 'h':
				return 0;
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

	server_socket ss;
	if (!ss.open(ip.c_str(), port)) {
		printf("Listen on %s:%d error\r\n", ip.c_str(), port);
		return 1;
	}

	printf("Listen on %s:%d ok\r\n", ip.c_str(), port);

	net_event ev(102400);

	server_proc proc(ev, ss, timeout);
	proc.read_await();

	while (!proc.stopped()) {
		ev.wait(1000);
	}
	return 0;
}