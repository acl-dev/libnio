//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#pragma once

struct NET_FILE;

namespace ev {

class net_event;

class event_proc {
public:
	explicit event_proc(net_event &ev, int fd);
	virtual  ~event_proc();

	bool read_await();
	bool write_await();
	void disable_read();
	void disable_write();

	virtual bool read() { return false; }
	virtual bool write() { return false; }
	virtual void destroy() {}

public:
	net_event &get_event() const {
		return ev_;
	}

private:
	net_event& ev_;
	NET_FILE*  fe_;

	static void read_proc(NET_EVENT *ev, NET_FILE *fe);
	static void write_proc(NET_EVENT *ev, NET_FILE *fe);
};

} // namespace
