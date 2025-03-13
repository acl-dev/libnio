# The fast and easy to use non-blocking network library

## 1. About
libnio is a high-performance, easy-to-use non-blocking network IO library that supports multiple platforms and events, such as poll, select, epoll, kqueue, iocp, and Windows GUI.

libnio contains C and C++11 code. The C part implements a general IO event library, and the C++11 part encapsulates the C part and provides more powerful functions.

## 2. Examples

Below is server example using libnio:

### 2.1. Server example

```C++
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "nio/nio_event.hpp"
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
        ssize_t ret = cli->read(fd, buf, sizeof(buf));
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

int main() {
    std::string ip("127.0.0.1");
    int port = 8288, timeout = 5000;
    nio_event_t etype = NIO_EVENT_T_KERNEL;

    // Ignore SIGPIPE to avoid process exit.
    signal(SIGPIPE, SIG_IGN);

    nio_event::debug(true);
    nio_event ev(102400, etype);

    server_socket server(ev);

    // Bind and listen the specified address.
    if (!server.open(ip.c_str(), port)) {
        printf("Listen on %s:%d error\r\n", ip.c_str(), port);
        return 1;
    }

    printf("Listen on %s:%d ok\r\n", ip.c_str(), port);

    // Register callback handlers in server.
    server.set_on_accept([&ev, timeout](socket_t fd, const std::string &addr) {
        printf("Accept on client from %s, fd: %d\r\n", addr.c_str(), fd);
        auto *cli = new client_socket(ev, fd);
        handle_client(cli, timeout);
    }).set_on_error([]() {
        printf("Error on server socket\r\n");
    }).set_on_close([]() {
        printf("Server socket closed\r\n");
    });

    // The server socket will accecpt client connections in async mode.
    server.accept_await();

    // IO event loop process.
    while (true) {
        ev.wait(1000);
    }

    return 0;
}
```

### 2.2. Client example

```C++
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <string>

#include "nio/nio_event.hpp"
#include "nio/client_socket.hpp"

using namespace nio;

static long long total_count = 10000;
static long long count = 0;
static int nconns = 0;

static bool connect_server(nio_event &ev, const char *ip, int port, int timeout) {
    auto *cli = new client_socket(ev);

    (*cli).on_connect([cli, timeout](socket_t fd, bool expired) {
        if (fd == -1 || expired) {
            printf("Connect failed, fd=%d, %s\r\n", fd,  expired ? "expired" : "error");
            cli->close_await();
            nconns--;
            return;
        }

        printf("Connect ok, fd %d\r\n", fd);
        const char *s = "hello world!\r\n";
        if (cli->write( s, strlen(s), timeout) == -1) {
            cli->close_await();
        } else {
            cli->read_await(timeout);
        }
    }).on_read([cli, timeout](socket_t fd, bool expired) {
        char buf[1024];
        ssize_t ret = cli->read(fd, buf, sizeof(buf));
        if (ret <= 0 || ++count >= total_count
            || cli->write(buf, ret, timeout) == -1) {
            cli->close_await();
        }
    }).on_write([cli](socket_t fd, bool expired) {
        printf("Write wait %s for fd %d\r\n", expired ? "expired" : "error", fd);
        if (expired) {
            cli->close_await();
        }
    }).on_error([cli](socket_t fd) {
        cli->close_await();
    }).on_close([cli](socket_t fd) {
        delete cli;
        nconns--;
    });

    if (!cli->connect_await(ip, port, 5000)) {
        delete cli;
        return false;
    }

    return true;
}

int main() {
    int cocurrent = 100;
    std::string ip("127.0.0.1");
    int port = 8288, timeout = 5000;
    nio_event_t etype = NIO_EVENT_T_KERNEL;

    signal(SIGPIPE, SIG_IGN);

    nio_event::debug(true);
    nio_event ev(102400, etype);

    for (int i = 0; i < cocurrent; i++) {
        if (!connect_server(ev, ip.c_str(), port, timeout)) {
            printf("Connect server error %s\r\n", strerror(errno));
            break;
        }
        ++nconns;
    }

    struct timeval begin{0, 0};
    gettimeofday(&begin, nullptr);

    while (nconns > 0) {
        ev.wait(1000);
    }

    return 0;
}
```

## 3. Build & install

- $ cd libnio; make
- $ cd libnio; sudo make install

## 4. Add libnio to your project

You should include `libnio.a and libnio_cpp.a` to your Makefile or CMakeLists.txt.
