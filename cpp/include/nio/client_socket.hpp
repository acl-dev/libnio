//
// Created by shuxin ¡¡¡¡zheng on 2025/2/19.
//

#pragma once
#include <functional>
#include "nio_define.hpp"
#include "event_timer.hpp"

struct NIO_EVENT;
struct NIO_FILE;

namespace nio {

class nio_event;
class client_timer;

using connect_handler_t = std::function<void(socket_t, bool)>;
using read_handler_t    = std::function<void(socket_t, bool)>;
using write_handler_t   = std::function<void(socket_t, bool)>;
using error_handler_t   = std::function<void(socket_t)>;
using close_handler_t   = std::function<void(socket_t)>;

class client_socket {
public:
    explicit client_socket(nio_event &ev);
    client_socket(nio_event &ev, socket_t fd);
    ~client_socket();

    bool connect_await(const char *ip, int port, int ms = -1);
    bool read_await(int ms = -1);
    bool write_await(int ms = -1);
    void read_disable();
    void write_disable();

    void close_await();

    bool is_closing() const {
        return closing_;
    }

    client_socket &on_connect(connect_handler_t fn);
    client_socket &on_read(read_handler_t fn);
    client_socket &on_write(write_handler_t fn);
    client_socket &on_error(error_handler_t fn);
    client_socket &on_close(close_handler_t fn);

public:
    ssize_t write(const void *data, size_t len, int ms = -1);

private:
    friend class nio_event;

    nio_event &ev_;
    connect_handler_t on_connect_;
    read_handler_t    on_read_;
    write_handler_t   on_write_;
    error_handler_t   on_error_;
    close_handler_t   on_close_;

    NIO_FILE *fe_  = nullptr;
    bool closing_  = false;
    void close();

    static void connect_callback(NIO_EVENT *ev, NIO_FILE *fe);
    static void read_callback(NIO_EVENT *ev, NIO_FILE *fe);
    static void write_callback(NIO_EVENT *ev, NIO_FILE *fe);

    ssize_t flush();

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
