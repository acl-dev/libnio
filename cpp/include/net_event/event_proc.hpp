//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#pragma once

struct NET_FILE;

namespace nev {

class net_event;

class event_proc {
public:
	explicit event_proc(net_event &ev, int fd);
	virtual ~event_proc();

	bool read_await();
	bool write_await();
	void read_disable();
	void write_disable();
	bool connect_await();

public:
	ssize_t write(const void *data, size_t len);
	void close();

	net_event &get_event() const {
		return ev_;
	}

	bool is_closing() const {
		return closing_;
	}

protected:
	virtual void on_read() {}
	virtual void on_write() {}
	virtual void on_connect(bool ok) {}
	virtual void on_error() { this->close(); }

public:
	virtual void on_close() {}

private:
	net_event&   ev_;
	NET_FILE*    fe_;
	std::string* buf_ = nullptr;
	bool closing_ = false;

	ssize_t flush();

	static void read_proc(NET_EVENT *ev, NET_FILE *fe);
	static void write_proc(NET_EVENT *ev, NET_FILE *fe);
	static void connect_proc(NET_EVENT *ev, NET_FILE *fe);
};

} // namespace
