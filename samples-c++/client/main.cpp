//
// Created by shuxin ¡¡¡¡zheng on 2025/2/17.
//
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <csignal>
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

#include "net_event/net_event.hpp"
#include "net_event/event_proc.hpp"
#include "net_event/event_timer.hpp"

using namespace nev;

static long long total_count = 10000;
static long long count = 0;
static int nconns = 0;

class client_proc : public event_proc, public event_timer {
public:
    client_proc(net_event &ev,int fd, int timeout)
    : event_proc(ev, fd), fd_(fd) , timeout_(timeout)
    {
        if (timeout > 0) {
            this->get_event().add_timer(this, timeout);
        }
    }

    ~client_proc() override {
        printf("Close fd %d\r\n", fd_);
        ::close(fd_);

        if (--nconns == 0) {
            printf("All connections closed!\r\n");
        }
    }

protected:
    // @override
    void on_connect(bool ok) override {
        if (!ok) {
            this->close();
            return;
        }

        const char *s = "hello world!\r\n";
        if (this->write(s, strlen(s)) == -1) {
            printf(">>>>>>write error: %s\r\n", strerror(errno));
            this->close();
        } else {
            this->read_await();
        }
    }

    // @override from event_proc
    void on_read() override {
        char buf[1024];
        ssize_t ret = ::read(fd_, buf, sizeof(buf));
        if (ret <= 0) {
            printf(">>>read error %s, ret=%ld\r\n", strerror(errno), ret);
            this->close();
            return;
        }

        if (++count >= total_count) {
            this->close();
            return;
        }

        if (this->write(buf, (size_t) ret) == -1) {
            this->close();
            return;
        }

        if (timeout_ > 0) {
            this->get_event().reset_timer(this, timeout_);
        }
    }

    // @override from event_timer
    void on_timer() override {
        printf("Read timeout from fd: %d\r\n", fd_);
        const char *s = "Timeout, bye!\r\n";
        ::write(fd_, s, strlen(s));
        this->close();
    }

    // @override from event_proc
    void on_error() override {
        this->close();
    }

    // @override from event_proc
    void on_close() override {
        if (timeout_ > 0) {
            this->get_event().del_timer(this);
        }
        delete this;
    }

private:
    int fd_;
    int timeout_;
};

static bool connect_server(net_event &ev, const char *ip, int port, int timeout) {
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family      = PF_INET;
    sa.sin_port        = htons(port);
    sa.sin_addr.s_addr = inet_addr(ip);

    int fd = socket(PF_INET, SOCK_STREAM, 0);
    net_event::set_nblock(fd, true);
    net_event::set_ndelay(fd, true);
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (connect(fd, (const struct sockaddr*) &sa, sizeof(sa)) < 0) {
        if (errno != EINPROGRESS && errno != EISCONN) {
            close(fd);
            printf(">>>Connect error %s\r\n", strerror(errno));
            return false;
        }
    }

    event_proc *proc = new client_proc(ev, fd, timeout);
    if (!proc->connect_await()) {
        printf("Write await for connecting error %s, fd %d\r\n", strerror(errno), fd);
        delete proc;
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
