//
// Created by shuxin zheng on 2025/2/17.
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

#include "nio/nio_event.hpp"
#include "nio/client_socket.hpp"
#include "nio/mqtt/mqtt_client.hpp"
#include "nio/mqtt/mqtt_message.hpp"
#include "nio/mqtt/mqtt_pingreq.hpp"

using namespace nio;

static long long total_count = 10000;
static long long count = 0;
static int nconns = 0;

static void handle_client(client_socket *cli, int timeout) {
    auto *conn = new mqtt_client(*cli);

    // Handle message from server when getting one message.
    conn->on_message([conn](const mqtt_message &msg) {
        ++count;

        if (count < 10) {
            printf("Received message on client fd %d\r\n", conn->get_conn().sock_handle());
        }

        if (count >= total_count) {
            printf("Total count reached: %lld\r\n", count);
            return false;
        }

        mqtt_pingreq pingreq;
        if (!conn->send_await(pingreq)) {
            printf("Failed to send PINGREQ to client fd %d\r\n", conn->get_conn().sock_handle());
            return false;
        }
        return true;
    }).on_timeout([](mqtt_client &client) {
        printf("Timeout for client fd %d\r\n", client.get_conn().sock_handle());
        return false;
    });

    // Reset the on_close handler here.
    (*cli).on_close([cli, conn](socket_t fd) {
        printf("Closing client fd %d, delete cli and conn\r\n", fd);
        delete cli;
        delete conn;
        nconns--;
        return true;
    });

    mqtt_pingreq pingreq;

    // Send the first message to server and read await the reply from server.
    if (!conn->send_await(pingreq)) {
        printf("Failed to send PINGREQ to client fd %d\r\n", cli->sock_handle());
        cli->close_await();
    } else if (!conn->read_await(timeout)) {
        printf("Failed to read from client fd %d\r\n", cli->sock_handle());
        cli->close_await();
    }

    printf("Client fd %d read waiting\r\n", cli->sock_handle());
}

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
        handle_client(cli, timeout);
    }).on_write([cli](socket_t fd, bool expired) {
        printf("Write wait %s for fd %d\r\n", expired ? "expired" : "error", fd);
        if (expired) {
            cli->close_await();
        }
    }).on_error([cli](socket_t fd) {
        cli->close_await();
    }).on_close([cli](socket_t fd) {
        printf("Closing client fd %d, delete cli\r\n", fd);
        delete cli;
        nconns--;
        return true;
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
    nio_event_t etype = NIO_EVENT_T_KERNEL;

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

    nio_event::debug(true);
    nio_event ev(102400, etype);

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
