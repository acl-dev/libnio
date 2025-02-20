//
// Created by shuxin ¡¡¡¡zheng on 2025/2/19.
//

#pragma once
#include <functional>
#include "net_define.hpp"
#include "event_timer.hpp"

struct NET_EVENT;
struct NET_FILE;

namespace nev {

class net_event;
class client_timer;

using connect_handler_t = std::function<void(socket_t, bool)>;
using read_handler_t    = std::function<void(socket_t, bool)>;
using write_handler_t   = std::function<void(socket_t, bool)>;
using error_handler_t   = std::function<void(socket_t)>;
using close_handler_t   = std::function<void(socket_t)>;

class client_socket {
public:
	explicit client_socket(net_event &ev);
	client_socket(net_event &ev, socket_t fd);
	~client_socket();

	bool connect_await(const char *ip, int port, int ms = -1);
	bool read_await(int ms = -1);
	bool write_await(int ms = -1);
	void close();

	client_socket &on_connect(connect_handler_t fn);
	client_socket &on_read(read_handler_t fn);
	client_socket &on_write(write_handler_t fn);
	client_socket &on_error(error_handler_t fn);
	client_socket &on_close(close_handler_t fn);

public:
	ssize_t write(const void *data, size_t len, int ms = -1);

private:
	net_event &ev_;
	connect_handler_t on_connect_;
	read_handler_t    on_read_;
	write_handler_t   on_write_;
	error_handler_t   on_error_;
	close_handler_t   on_close_;

	NET_FILE *fe_       = nullptr;

	static void connect_callback(NET_EVENT *ev, NET_FILE *fe);
	static void read_callback(NET_EVENT *ev, NET_FILE *fe);
	static void write_callback(NET_EVENT *ev, NET_FILE *fe);

private:
	client_timer *read_timer_  = nullptr;
	client_timer *write_timer_ = nullptr;
	std::string  *buf_         = nullptr;

	friend class client_timer;
	void on_timer(client_timer *timer);
};

////////////////////////////////////////////////////////////////////////////////

class client_timer : public event_timer {
public:
	explicit client_timer(client_socket &cli) : cli_(cli) {}
	~client_timer() override = default;

	bool is_waiting() const {
		return waiting_;
	}

	void set_waiting(bool yes) {
		waiting_ = yes;
	}

	bool is_connecting() const {
		return connecting_;
	}

	void set_connecting(bool yes) {
		connecting_ = yes;
	}

protected:
	void on_timer() override {
		cli_.on_timer(this);
	}
private:
	client_socket &cli_;
	bool waiting_    = false;
	bool connecting_ = false;
};

} // namespace
