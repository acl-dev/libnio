//
// Created by shuxin zheng on 2025/2/17.
//

#pragma once
#include <functional>
#include <string>
#include "nio_define.hpp"
#include "event_proc.hpp"

namespace nio {

// The handler when one socket is accepted.
using accept_handler_t = std::function<void(socket_t, const std::string &)>;

// The handler when an error occurs.
using accept_error_handler_t = std::function<void()>;

// The handler when the server socket is closed.
using server_close_handler_t = std::function<void()>;

/**
 * @brief The server socket class for server socket operations.
 */
class server_socket : public event_proc {
public:
    explicit server_socket(int backlog = 128);
    explicit server_socket(nio_event &ev, int backlog = 128);
    ~server_socket() override;

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
    /**
     * @brief Accept a client connection in async mode.
     * @return Return true if the accept operation is successful.
     */
    bool accept_await();

    /**
     * @brief Set the on_accept handler when a client is accepted.
     * @param fn The on_accept handler.
     * @return server_socket& 
     */
    server_socket &set_on_accept(accept_handler_t fn);

    /**
     * @brief Set the on_error handler when an error occurs.
     * @param fn The on_error handler.
     * @return server_socket& 
     */
    server_socket &set_on_error(accept_error_handler_t fn);

    /**
     * @brief Set the on_close handler when the server socket is closed.
     * @param fn The on_close handler.
     * @return server_socket& 
     */
    server_socket &set_on_close(server_close_handler_t fn);

protected:
    // @overload
    void on_read() override;

    // @overload
    void on_close() override;

private:
    socket_t lfd_ = -1;
    int      backlog_;

    accept_handler_t on_accept_;
    accept_error_handler_t  on_error_;
    server_close_handler_t  on_close_;
};

} // namespace
