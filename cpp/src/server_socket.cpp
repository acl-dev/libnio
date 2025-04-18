//
// Created by shuxin zheng on 2025/2/17.
//

#include "stdafx.hpp"
#include "nio/server_socket.hpp"

namespace nio {

server_socket::server_socket(int backlog)
: backlog_(backlog)
{
}

server_socket::~server_socket() {
    if (lfd_ >= 0) {
        close(lfd_);
    }
}

bool server_socket::open(const char *ip, int port) {
    struct sockaddr_in sa;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family      = PF_INET;
    sa.sin_port        = htons(port);
    sa.sin_addr.s_addr = inet_addr(ip);

    lfd_ = socket(PF_INET, SOCK_STREAM, 0);
    if (lfd_ == -1) {
        return false;
    }

    int on = 1;
    setsockopt(lfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (::bind(lfd_, (const struct sockaddr*) &sa, sizeof(sa)) < 0) {
        ::close(lfd_);
        lfd_ = -1;
        return false;
    }
    if (::listen(lfd_, backlog_) < 0) {
        lfd_ = -1;
        ::close(lfd_);
        return false;
    }
    return true;
}

socket_t server_socket::accept(std::string *addr) const {
    SOCK_ADDR saddr;
    memset(&saddr, 0, sizeof(saddr));

    auto *sa = (struct sockaddr *) &saddr;
    auto len = (socklen_t) sizeof(saddr);
    socket_t fd = ::accept(lfd_, sa, &len);
    if (fd == -1) {
        return -1;
    }

    if (addr == nullptr) {
        return fd;
    }

    if (sa->sa_family == PF_INET) {
        char ip[32];
        if (!inet_ntop(sa->sa_family, &saddr.in.sin_addr, ip, sizeof(ip))) {
            ::close(fd);
            return -1;
        }
        int port = ntohs(saddr.in.sin_port);
        std::stringstream buf;
        buf << ip << ":" << port;
        *addr = buf.str();
    } else if (sa->sa_family == PF_INET6) {
        char ip[64];
        if (!inet_ntop(sa->sa_family, &saddr.in6.sin6_addr, ip, sizeof(ip))) {
            ::close(fd);
            return -1;
        }
        char ifname[IF_NAMESIZE];
        char *ptr = (char *) if_indextoname(saddr.in6.sin6_scope_id, ifname);
        if (ptr == nullptr) {
            ifname[0] = 0;
        }

        std::stringstream buf;
        int port = ntohs(saddr.in6.sin6_port);
        if (port <= 0) {
            if (strcmp(ip, "::1") != 0 && ifname[0] != 0) {
                buf << ip << '%' << ifname;
            } else {
                buf << ip;
            }
        } else if (strcmp(ip, "::1") != 0 && ifname[0] != 0) {
            buf << ip << '%' << ifname << '|' << port;
        } else {
            buf << ip << '|' << port;
        }
        *addr = buf.str();
    }

    return fd;
}

bool server_socket::accept_await(nio_event &ev) {
    if (lfd_ < 0) {
        return false;
    }

    this->bind(ev, lfd_);
    return this->read_await();
}

server_socket &server_socket::set_on_accept(accept_handler_t fn) {
    on_accept_ = std::move(fn);
    return *this;
}

server_socket &server_socket::set_on_error(accept_error_handler_t fn) {
    on_error_ = std::move(fn);
    return *this;
}

server_socket &server_socket::set_on_close(server_close_handler_t fn) {
    on_close_ = std::move(fn);
    return *this;
}

void server_socket::on_read() {
    std::string addr;
    int fd = this->accept(&addr);
    if (fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && on_error_) {
            on_error_();
        }
    } else if (on_accept_) {
        on_accept_(fd, addr);
    }
}

void server_socket::on_close() {
    if (on_close_) {
        on_close_();
    }
}

} // namespace nio
