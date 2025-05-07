//
// Created by shuxin zheng on 2025/2/14.
//

#include "stdafx.hpp"
#include "nio/nio_event.hpp"
#include "nio/event_proc.hpp"

namespace nio {

event_proc::event_proc(nio_event &ev, socket_t fd) : ev_(&ev) {
	fe_ = nio_file_alloc(fd);
	nio_file_set_ctx(fe_, this);
}

event_proc::~event_proc() {
    if (fe_) {
        nio_file_free(fe_);
    }
    delete buf_;
}

void event_proc::bind(nio_event &ev, socket_t fd) {
    ev_ = &ev;

    if (fe_) {
        nio_file_free(fe_);
    }
    fe_ = nio_file_alloc(fd);
    nio_file_set_ctx(fe_, this);
}

socket_t event_proc::sock_handle() const {
    return fe_ ? fe_->fd : invalid_socket;
}

void event_proc::set_closing() {
    closing_ = true;
}

void event_proc::close_await() {
    if (ev_ == nullptr) {
        return;
    }

    if (!closing_) {
        nio_event_del_readwrite(ev_->get_event(), fe_);
        ev_->delay_close(this);
        closing_ = true;
    }
}

bool event_proc::read_await() {
    if (ev_ == nullptr) {
        return false;
    }

    return nio_event_add_read(ev_->get_event(), fe_, read_proc) != 0;
}

bool event_proc::write_await() {
    if (ev_ == nullptr) {
        return false;
    }

    return nio_event_add_write(ev_->get_event(), fe_, write_proc) != 0;
}

void event_proc::read_disable() {
    if (ev_ == nullptr) {
        return;
    }

    nio_event_del_read(ev_->get_event(), fe_);
}

void event_proc::write_disable() {
    if (ev_ == nullptr) {
        return;
    }

    nio_event_del_write(ev_->get_event(), fe_);
}

bool event_proc::connect_await() {
    if (ev_ == nullptr) {
        return true;
    }

    return nio_event_add_write(ev_->get_event(), fe_, connect_proc) != 0;
}

ssize_t event_proc::write(const void *data, size_t len) {
    // If last data hasn't been flush, just append it to the buffer.
    if (buf_ && !buf_->empty()) {
        buf_->append((const char*) data, len);
        return 0;
    }
#if 1
    ssize_t ret = ::write(fe_->fd, data, len);
    if (ret == (ssize_t) len) {
        return ret;
    }
#else
    ssize_t ret = 0;
#endif

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

void event_proc::read_proc(NIO_EVENT *, NIO_FILE *fe) {
    auto* me = (event_proc*) nio_file_get_ctx(fe);
    assert(me);
    me->on_read();
}

void event_proc::write_proc(NIO_EVENT *, NIO_FILE *fe) {
    auto* me = (event_proc*) nio_file_get_ctx(fe);
    assert(me);

    if (me->buf_ && !me->buf_->empty()) {
        ssize_t ret = me->flush();
        if (ret == -1) {
            me->on_error();
            return;
        }
        if (ret == 0 || !me->buf_->empty()) {
            return;
        }
    }

    me->on_write();
}

void event_proc::connect_proc(NIO_EVENT *, NIO_FILE *fe) {
    auto* me = (event_proc*) nio_file_get_ctx(fe);
    assert(me);

    me->write_disable();

    int err = 0;
    socklen_t len = sizeof(err);
    int ret = getsockopt(fe->fd, SOL_SOCKET, SO_ERROR, (char*) &err, &len);
    if (ret == 0 && (err == 0 || err == EISCONN)) {
        me->on_connect(true);
    } else {
        me->on_connect(false);
    }
}

} // namespace
