# The fast and easy to use non-blocking network library

## About
libnio is a high-performance, easy-to-use non-blocking network IO library that supports multiple platforms and events, such as poll, select, epoll, kqueue, iocp, and Windows GUI.

libnio contains C and C++11 code. The C part implements a general IO event library, and the C++11 part encapsulates the C part and provides more powerful functions.

## Examples

Below is server example using libnio:

```C++
#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "nio/nio_event.hpp"
#include "nio/event_proc.hpp"
#include "nio/client_socket.hpp"
#include "nio/server_socket.hpp"

using namespace nio;

// Handle client IO process in async mode.
static void handle_client(client_socket *cli, int timeout) {
    (*cli).on_read([cli, timeout](socket_t fd, bool expired) {
        if (expired) {
            printf("Read timeout for fd %d\r\n", fd);
            cli->close_await();
            return;
        }

        char buf[1204];
        ssize_t ret = ::read(fd, buf, sizeof(buf));
        if (ret <= 0 || cli->write(buf, ret, timeout) == -1) {
            cli->close_await();
        }
    }).on_write([cli](socket_t fd, bool expired) {
        if (expired) {
            printf("Write expired for fd %d\r\n", fd);
            cli->close_await();
        }
    }).on_error([cli](socket_t fd) {
        cli->close_await();
    }).on_close([cli](socket_t fd) {
        printf("Closing client fd %d\r\n", fd);
        delete cli;
    });

    // Add async read event.
    cli->read_await(timeout);
}

// The server socket will accept client connections in async mode.
class server_proc : public server_socket {
public:
    server_proc(nio_event &ev, int timeout)
    : server_socket(ev), timeout_(timeout) {}

    ~server_proc() override  = default;

    bool stopped() const { return stop_; }

protected:
    // @override
    void on_accept(socket_t fd, const std::string &addr) override {
        printf("Accept on client from %s, fd: %d\r\n", addr.c_str(), fd);

        auto *cli = new client_socket(*this->get_event(), fd);
        handle_client(cli, timeout_);
    }

    // @override
    void on_close() override {
        stop_ = true;
    }

private:
    bool stop_ = false;
    int timeout_;
};

int main() {
    std::string ip("127.0.0.1");
    int port = 8288, timeout = 5000;
    nio_event_t etype = NIO_EVENT_T_KERNEL;

    // Ignore SIGPIPE to avoid process exit.
    signal(SIGPIPE, SIG_IGN);

    nio_event::debug(true);
    nio_event ev(102400, etype);

    server_proc server(ev, timeout);

    // Bind and listen the specified address.
    if (!server.open(ip.c_str(), port)) {
        printf("Listen on %s:%d error\r\n", ip.c_str(), port);
        return 1;
    }

    printf("Listen on %s:%d ok\r\n", ip.c_str(), port);

    // The server socket will accecpt client connections in async mode.
    server.accept_await();

    // IO event loop process.
    while (!server.stopped()) {
        ev.wait(1000);
    }

    return 0;
}
```

## Build & install

- $ cd libnio; make
- $ cd libnio; sudo make install

## Add libnio to your project

You should include `libnio.a and libnio_cpp.a` to your Makefile or CMakeLists.txt.