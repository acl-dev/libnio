//
// Created by shuxin zheng on 2025/2/17.
//
#include <getopt.h>
#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "nio/nio_event.hpp"
#include "nio/event_proc.hpp"
#include "nio/event_timer.hpp"
#include "nio/server_socket.hpp"

using namespace nio;

class client_proc : public event_proc, public event_timer {
public:
    client_proc(nio_event &ev, int fd, int timeout)
    : event_proc(ev, fd), fd_(fd) , timeout_(timeout)
    {
        if (timeout > 0) {
            this->get_event()->add_timer(this, timeout);
        }
    }

    ~client_proc() override {
        printf("Close fd %d\r\n", fd_);
        ::close(fd_);
    }

protected:
    // @override from event_proc
    void on_read() override {
        char buf[4096];
        ssize_t ret = ::read(fd_, buf, sizeof(buf));
        if (ret <= 0) {
            this->close_await();
            return;
        }

#if 0
        if (::write(fd_, buf, (size_t) ret) != ret) {
#else
        if (this->write(buf, (size_t) ret) == -1) {
#endif
            this->close_await();
            return;
        }

        if (timeout_ > 0) {
            this->get_event()->reset_timer(this, timeout_);
        }
    }

    // @override from event_timer
    void on_timer() override {
        printf("Read timeout from fd: %d\r\n", fd_);
        const char *s = "Timeout, bye!\r\n";
        ::write(fd_, s, strlen(s));
        this->close_await();
    }

    // @override from event_proc
    void on_error() override {
        this->close_await();
    }

    // @override from event_proc
    void on_close() override {
        if (timeout_ > 0) {
            this->get_event()->del_timer(this);
        }
        delete this;
    }

private:
    int fd_;
    int timeout_;
};

class server_proc : public event_proc {
public:
    server_proc(nio_event &ev, server_socket &ss, int timeout)
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
        event_proc *proc = new client_proc(*this->get_event(), fd, timeout_);
        if (!proc->read_await()) {
            printf("Read await for fd %d error\r\n", fd);
            delete proc;
        }
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
    nio_event_t etype = NIO_EVENT_T_KERNEL;

    while ((ch = getopt(argc, argv, "he:l:p:t:")) > 0) {
        switch (ch) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 'e':
            if (strcmp(optarg, "poll") == 0) {
                etype = NIO_EVENT_T_POLL;
                printf("Use pool\r\n");
            } else if (strcmp(optarg, "select") == 0) {
                etype = NIO_EVENT_T_SELECT;
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

    nio_event::debug(true);
    nio_event ev(102400, etype);

    server_proc proc(ev, ss, timeout);
    proc.read_await();

    while (!proc.stopped()) {
        ev.wait(1000);
    }

    printf(">>>>Server stopped!<<<<\r\n");
    return 0;
}
