//
// Created by shuxin zheng on 2025/2/17.
//

#pragma once
#include <string>
#include "nio_define.hpp"
#include "event_proc.hpp"

namespace nio {

/**
 * @brief The server socket class for server socket operations.
 */
class server_socket : public event_proc {
public:
    explicit server_socket(int backlog = 128);
    explicit server_socket(nio_event &ev, int backlog = 128);
    virtual ~server_socket() override;

    /**
     * @brief Bind and listen on the specified IP and port.
     * @param ip The server IP address.
     * @param port The server port number.
     * @return Return true if the server socket is opened successfully.
     */
    bool open(const char *ip, int port);

    /**
     * @brief Accept a client connection in sync mode.
     * @param addr The client address to return.
     * @return Return the client socket file descriptor, or -1 if an error occurs.
     */
    socket_t accept(std::string *addr = nullptr) const;

    /**
     * @brief Get the server socket file descriptor.
     * @return Return the server socket file descriptor.
     */
    socket_t sock_handle() const { return lfd_; }

public:
    void accept_await();

protected:
    virtual void on_accept(socket_t fd, const std::string &addr) {}

    // @overload
    void on_read() override;

private:
    socket_t lfd_ = -1;
    int backlog_;
};

} // namespace
