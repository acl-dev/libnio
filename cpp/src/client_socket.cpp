//
// Created by shuxin zheng on 2025/2/19.
//

#include "stdafx.hpp"
#include "nio/nio_event.hpp"
//#include "nio/event_proc.hpp"
#include "nio/client_socket.hpp"

namespace nio {

client_socket::client_socket(nio_event &ev) : ev_(ev) {}
client_socket::client_socket(nio_event &ev, socket_t fd)
: ev_(ev)
{
    fe_ = nio_file_alloc(fd);
    fe_->ctx = this;
}

client_socket::~client_socket() {
    if (fe_) {
        nio_file_free(fe_);
    }
    delete read_timer_;
    delete write_timer_;
    delete buf_;
}

socket_t client_socket::sock_handle() const {
    return fe_ ? fe_->fd : invalid_socket;
}

void client_socket::read_disable() const {
    assert(fe_);
    nio_event_del_read(ev_.get_event(), fe_);
}

void client_socket::write_disable() const {
    assert(fe_);
    nio_event_del_write(ev_.get_event(), fe_);
}

void client_socket::readwrite_disable() const {
    assert(fe_);
    nio_event_del_readwrite(ev_.get_event(), fe_);
}

bool client_socket::connect_await(const char *ip, int port, int ms /* -1 */) {
    sockaddr_in sa{};
    memset(&sa, 0, sizeof(sa));
    sa.sin_family      = PF_INET;
    sa.sin_port        = htons(port);
    sa.sin_addr.s_addr = inet_addr(ip);

    int fd = socket(PF_INET, SOCK_STREAM, 0);
    nio_event::set_nblock(fd, true);
    nio_event::set_ndelay(fd, true);
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (::connect(fd, reinterpret_cast<const sockaddr*>(&sa), sizeof(sa)) < 0) {
        if (errno != EINPROGRESS && errno != EISCONN) {
            ::close(fd);
            return false;
        }
    }
    NIO_FILE *fe = nio_file_alloc(fd);
    if (!nio_event_add_write(ev_.get_event(), fe, connect_callback)) {
        ::close(fd);
        nio_file_free(fe);
        return false;
    }
    fe->ctx = this;
    fe_ = fe;
    if (ms < 0) {
        return true;
    }

    if (write_timer_ == nullptr) {
        write_timer_ = new client_timer(*this);
    }
    ev_.add_timer(write_timer_, ms);
    write_timer_->set_connecting(true);
    write_timer_->set_waiting(true);
    return true;
}

void client_socket::connect_callback(NIO_EVENT *ev, NIO_FILE *fe) {
    auto *cli = static_cast<client_socket*>(fe->ctx);
    assert(cli);
    cli->flags_ &= ~client_f_wtimer;

    if (cli->write_timer_ && cli->write_timer_->is_connecting()) {
        cli->write_timer_->set_waiting(false);
        cli->write_timer_->set_connecting(false);
        cli->ev_.del_timer(cli->write_timer_);
    }

    int err = 0;
    socklen_t len = sizeof(err);
    int ret = getsockopt(fe->fd, SOL_SOCKET, SO_ERROR, (char*) &err, &len);
    if (ret == 0 && (err == 0 || err == EISCONN)) {
        nio_event_del_write(ev, fe);
        cli->on_connect_(fe->fd, false);
    } else {
        nio_event_del_readwrite(ev, fe);
        cli->on_connect_(-1, false);
    }
}

bool client_socket::read_await(int ms /* -1 */) {
    assert(fe_);
    if (!ev_.isset_oneshot() || (flags_ & client_f_rtimer) == 0) {
        if (!nio_event_add_read(ev_.get_event(), fe_, read_callback)) {
            return false;
        }
    }

    if (ms <= 0) {
        return true;
    }

    if (read_timer_ == nullptr) {
        read_timer_ = new client_timer(*this);
    }

    if (!read_timer_->is_waiting()) {
        ev_.add_timer(read_timer_, ms);
        read_timer_->set_waiting(true);
    }
    return true;
}

void client_socket::read_callback(NIO_EVENT *, NIO_FILE *fe) {
    auto *cli = static_cast<client_socket*>(fe->ctx);
    assert(cli);
    cli->flags_ &= ~client_f_rtimer;

    if (cli->read_timer_ && cli->read_timer_->is_waiting()) {
        cli->ev_.del_timer(cli->read_timer_);
        cli->read_timer_->set_waiting(false);
    }
    if (cli->on_read_) {
        cli->on_read_(fe->fd, false);
    }
}

bool client_socket::write_await(int ms /* -1 */) {
    assert(fe_);
    if (!ev_.isset_oneshot() || (flags_ & client_f_wtimer) == 0) {
        if (!nio_event_add_write(ev_.get_event(), fe_, write_callback)) {
            return false;
        }
    }

    if (ms <= 0) {
        return true;
    }

    if (write_timer_ == nullptr) {
        write_timer_ = new client_timer(*this);
    }

    if (!write_timer_->is_waiting()) {
        ev_.add_timer(write_timer_, ms);
        write_timer_->set_waiting(true);
    }
    return true;
}

void client_socket::write_callback(NIO_EVENT *, NIO_FILE *fe) {
    auto *cli = static_cast<client_socket*>(fe->ctx);
    assert(cli);
    cli->flags_ &= ~client_f_wtimer;

    if (cli->write_timer_ && cli->write_timer_->is_waiting()) {
        cli->ev_.del_timer(cli->write_timer_);
        cli->write_timer_->set_waiting(false);
    }

    if (cli->buf_ && !cli->buf_->empty()) {
        ssize_t ret = cli->flush();
        if (ret == -1) {
            if (cli->on_error_) {
                cli->on_error_(cli->fe_->fd);
            }
            return;
        }
        if (ret == 0 || cli->buf_->empty()) {
            return;
        }
    }

    if (cli->on_write_) {
        cli->on_write_(fe->fd, false);
    }
}

ssize_t client_socket::flush() const {
    if (!buf_ || buf_->empty()) {
        return 0;
    }

    ssize_t ret = ::write(fe_->fd, buf_->c_str(), buf_->size());
    if (ret == static_cast<ssize_t>(buf_->size())) {
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

void client_socket::on_timer(client_timer *timer) {
    if (timer == read_timer_) {
        timer->set_waiting(false);
        flags_ |= client_f_rtimer;
        if (on_read_) {
            on_read_(fe_->fd, true);
        }
        flags_ &= ~client_f_rtimer;
    } else if (timer == write_timer_) {
        timer->set_waiting(false);
        flags_ |= client_f_wtimer;
        if (timer->is_connecting()) {
            timer->set_connecting(false);
            if (on_connect_) {
                on_connect_(fe_->fd, true);
            }
        } else if (on_write_) {
            on_write_(fe_->fd, true);
        }
        flags_ &= ~client_f_wtimer;
    } else {
        assert(0);
    }
}

void client_socket::set_closing(const bool yes) {
    closing_ = yes;
}

void client_socket::close_await() {
    if (!closing_) {
        if (fe_ && fe_->fd >= 0) {
            nio_event_del_readwrite(ev_.get_event(), fe_);
        }
        ev_.delay_close(this);
        closing_ = true;
    }
}

void client_socket::close() const {
    if (read_timer_ && read_timer_->is_waiting()) {
        ev_.del_timer(read_timer_);
        read_timer_->set_waiting(false);
    }
    if (write_timer_ && write_timer_->is_waiting()) {
        ev_.del_timer(write_timer_);
        write_timer_->set_waiting(false);
    }

    if (fe_ && fe_->fd >= 0) {
        const int fd = fe_->fd;
        if (on_close_ != nullptr) {
            if (on_close_(fe_->fd)) {
                ::close(fd);
            }
        } else {
            ::close(fd);
        }
    } else if (on_close_ != nullptr) {
        (void) on_close_(-1);
    }
}

client_socket &client_socket::on_connect(connect_handler_t fn) {
    on_connect_ = std::move(fn);
    return *this;
}

client_socket &client_socket::on_read(read_handler_t fn) {
    on_read_ = std::move(fn);
    return *this;
}

client_socket &client_socket::on_write(write_handler_t fn) {
    on_write_ = std::move(fn);
    return *this;
}

client_socket &client_socket::on_error(error_handler_t fn) {
    on_error_ = std::move(fn);
    return *this;
}

client_socket &client_socket::on_close(close_handler_t fn) {
    on_close_ = std::move(fn);
    return *this;
}

client_socket &client_socket::set_ctx(void *ctx) {
    ctx_ = ctx;
    return *this;
}

ssize_t client_socket::read(void *buf, size_t count) const {
    return ::read(fe_->fd, buf, count);
}

ssize_t client_socket::write(const void *data, size_t len, int ms) {
    if (buf_ && !buf_->empty()) {
        buf_->append(static_cast<const char*>(data), len);
        return 0;
    }

    ssize_t ret = ::write(fe_->fd, data, len);
    if (ret == static_cast<ssize_t>(len)) {
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

    buf_->append(static_cast<const char*>(data) + ret, len - ret);
    if (!write_await(ms)) {
        return -1;
    }
    return ret;
}

} // namespace
