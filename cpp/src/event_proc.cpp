//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#include "stdafx.hpp"
#include "net_event.hpp"
#include "event_proc.hpp"

namespace ev {

event_proc::event_proc(net_event &ev, int fd) : ev_(ev) {
	fe_ = net_file_alloc(fd);
	net_file_set_ctx(fe_,this);
}

event_proc::~event_proc() {
	net_file_free(fe_);
	delete buf_;
}

void event_proc::read_proc(NET_EVENT *, NET_FILE *fe) {
	auto* me = (event_proc*) net_file_get_ctx(fe);
	assert(me);
	if (!me->read()) {
		me->disable_read();
	}
}

void event_proc::write_proc(NET_EVENT *, NET_FILE *fe) {
	auto* me = (event_proc*) net_file_get_ctx(fe);
	assert(me);

	if (me->buf_ && !me->buf_->empty()) {
		ssize_t ret = me->flush();
		if (ret == -1) {
			me->write();
			me->disable_write();
			return;
		}
		if (ret == 0 || !me->buf_->empty()) {
			return;
		}
	}

	if (!me->write()) {
		me->disable_write();
	}
}

bool event_proc::read_await() {
	return net_event_add_read(ev_.get_event(), fe_, read_proc) != 0;
}

bool event_proc::write_await() {
	return net_event_add_write(ev_.get_event(), fe_, write_proc) != 0;
}

void event_proc::disable_read() {
	net_event_del_read(ev_.get_event(), fe_);
}

void event_proc::disable_write() {
	net_event_del_write(ev_.get_event(), fe_);
}

ssize_t event_proc::send(const void *data, size_t len) {
	if (buf_ && !buf_->empty()) {
		buf_->append((const char*) data, len);
		return 0;
	}
	ssize_t ret = ::write(fe_->fd, data, len);
	if (ret == (ssize_t) len) {
		return ret;
	}

	if (ret == -1) {
#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN) {
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
#endif
			return -1;
		}
	}

	if (!buf_) {
		buf_ = new std::string;
	}

	buf_->append((const char*) data + ret, len - ret);
	if (!write_await()) {
		return -1;
	}
	return ret;
}

ssize_t event_proc::flush() {
	if (!buf_ || buf_->empty()) {
		return 0;
	}

	ssize_t ret = ::write(fe_->fd, buf_->c_str(), buf_->size());
	if (ret == (ssize_t) buf_->size()) {
		buf_->clear();
		return ret;
	}

	if (ret == -1) {
#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN) {
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
#endif
			return -1;
		}
		return 0;
	}
	*buf_ = buf_->substr(ret, buf_->size());
	return ret;
}

} // namespace
