//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#pragma once

struct NIO_FILE;

namespace nio {

class nio_event;

class event_proc {
public:
    explicit event_proc(nio_event &ev, int fd);
    virtual ~event_proc();

    bool read_await();
    bool write_await();
    void read_disable();
    void write_disable();
    bool connect_await();
    void close_await();

public:
    ssize_t write(const void *data, size_t len);

    nio_event &get_event() const {
        return ev_;
    }

    bool is_closing() const {
        return closing_;
    }

protected:
    virtual void on_read() {}
    virtual void on_write() {}
    virtual void on_connect(bool ok) {}
    virtual void on_error() { this->close_await(); }

public:
    virtual void on_close() {}

private:
    nio_event&   ev_;
    NIO_FILE*    fe_;
    std::string* buf_ = nullptr;
    bool closing_ = false;

    ssize_t flush();

    static void read_proc(NIO_EVENT *ev, NIO_FILE *fe);
    static void write_proc(NIO_EVENT *ev, NIO_FILE *fe);
    static void connect_proc(NIO_EVENT *ev, NIO_FILE *fe);
};

} // namespace
