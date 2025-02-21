//
// Created by shuxin ¡¡¡¡zheng on 2025/2/17.
//
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>
#include <string>

#include "net_event/net_event.hpp"
#include "net_event/client_socket.hpp"

using namespace nev;

static long long total_count = 10000;
static long long count = 0;
static int nconns = 0;

static bool connect_server(net_event &ev, const char *ip, int port, int timeout) {
    auto *cli = new client_socket(ev);

    (*cli).on_connect([cli, timeout](socket_t fd, bool expired) {
        if (fd == -1 || expired) {
            printf("Connect failed, fd=%d, %s\r\n", fd,  expired ? "expired" : "error");
            cli->close();
            nconns--;
            return;
        }

        printf("Connect ok, fd %d\r\n", fd);
        const char *s = "hello world!\r\n";
        if (cli->write( s, strlen(s), timeout) == -1) {
            cli->close();
        } else {
            cli->read_await(timeout);
        }
    }).on_read([cli, timeout](socket_t fd, bool expired) {
        char buf[1024];
        ssize_t ret = ::read(fd, buf, sizeof(buf));
        if (ret <= 0 || ++count >= total_count
            || cli->write(buf, ret, timeout) == -1) {
            cli->close();
        }
    }).on_write([cli](socket_t fd, bool expired) {
        printf("Write wait %s for fd %d\r\n", expired ? "expired" : "error", fd);
        if (expired) {
            cli->close();
        }
    }).on_error([cli](socket_t fd) {
        cli->close();
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

static void usage(const char *procname) {
    printf("Usage: %s -h[help]\r\n"
           " -e event_type[kernel|poll|select, default: kernel]\r\n"
           " -c cocurrent[default: 100]\r\n"
           " -n total_count[default: 10000]\r\n"
           " -s server_ip[default: 127.0.0.1]\r\n"
           " -p server_port[default: 8288]\r\n"
           " -t IO timeout in ms[default: 5000]\r\n", procname);
}

int main(int argc, char *argv[]) {
    int ch, cocurrent = 100;
    std::string ip("127.0.0.1");
    int port = 8288, timeout = 5000;
    net_event_t etype = NET_EVENT_T_KERNEL;

    while ((ch = getopt(argc, argv, "he:c:n:l:p:t:")) > 0) {
        switch (ch) {
            case 'h':
                usage(argv[0]);
                return 0;
            case 'c':
                cocurrent = std::atoi(optarg);
                break;
            case 'n':
                total_count = std::atoll(optarg);
                break;
            case 'e':
                if (strcmp(optarg, "poll") == 0) {
                    etype = NET_EVENT_T_POLL;
                    printf("Use pool\r\n");
                } else if (strcmp(optarg, "select") == 0) {
                    etype = NET_EVENT_T_SELECT;
                    printf("Use select\r\n");
                } else if (strcmp(optarg, "kernel") != 0) {
                    printf("Unknown event: %s\r\n", optarg);
                    usage(argv[0]);
                    return 1;
                } else {
                    printf("Use kernel\r\n");
                }
                break;
            case 's':
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

    net_event::debug(true);
    net_event ev(102400, etype);

    for (int i = 0; i < cocurrent; i++) {
        if (!connect_server(ev, ip.c_str(), port, timeout)) {
            printf("Connect server error %s\r\n", strerror(errno));
            break;
        }
        ++nconns;
    }

    if (nconns == 0) {
        printf("No connection available!\r\n");
    }

    struct timeval begin{0, 0};
    gettimeofday(&begin, nullptr);

    while (nconns > 0) {
        ev.wait(1000);
    }

    struct timeval end{0, 0};
    gettimeofday(&end, nullptr);

    long long n = end.tv_sec * 1000000 + end.tv_usec
                  - begin.tv_sec * 1000000 - begin.tv_usec;
    auto tc = (double) (n / 1000);
    double speed = ((double) count * 1000)	/ (tc > 0 ? tc : 0.0000000000001);

    printf("Total count=%lld, tc=%.2f ms, speed=%.2f\r\n", count, tc, speed);
    return 0;
}
