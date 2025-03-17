//
// Created by shuxin zheng on 2025/2/19.
//

#pragma once
#include <functional>
#include "nio_define.hpp"
#include "event_timer.hpp"

struct NIO_EVENT;
struct NIO_FILE;

namespace nio {

class nio_event;
class client_timer;

// The handler when the socket is connected.
using connect_handler_t = std::function<void(socket_t, bool)>;

// The handler when the socket is readable.
using read_handler_t    = std::function<void(socket_t, bool)>;

// The handler when the socket is writable.
using write_handler_t   = std::function<void(socket_t, bool)>;

// The handler when the socket has an error.
using error_handler_t   = std::function<void(socket_t)>;

// The handler when the socket is closed.
using close_handler_t   = std::function<void(socket_t)>;

/**
 * @brief The client socket class for client socket read and write operations.
 */
class client_socket {
public:
    /**
     * @brief Constructor used when connecting to a server.
     * @param ev The nio event object.
     */
    explicit client_socket(nio_event &ev);

    /**
     * @brief Constructor when the socket is already connected.
     * @param ev The nio event object.
     * @param fd The connected socket file descriptor.
     */
    client_socket(nio_event &ev, socket_t fd);
    ~client_socket();

    /**
     * @brief Connect to a server with the specified IP and port in async mode,
     *  when the connection is established, the on_connect() handler will be called.
     * @param ip The server IP address.
     * @param port The server port number.
     * @param ms The timeout in milliseconds.
     * @return If the connection is successful.
     */
    bool connect_await(const char *ip, int port, int ms = -1);

    /**
     * @brief Read data from the socket in async mode, when the read operation
     *  is successful, the on_read() handler will be called.
     * @param ms The timeout in milliseconds.
     * @return If the read operation is successful.
     */
    bool read_await(int ms = -1);

    /**
     * @brief Write data to the socket in async mode, when the write operation
     *  is successful, the on_write() handler will be called.
     * @param ms The timeout in milliseconds.
     * @return If the write operation is successful.
     */
    bool write_await(int ms = -1);

    /**
     * @brief Disable the read event.
     */
    void read_disable();

    /**
     * @brief Disable the write event.
     */
    void write_disable();

    /**
     * @brief Close the socket in async mode.
     */
    void close_await();

    /**
     * @brief Set the socket's closing flag.
     */
    void set_closing();

    /**
     * @brief Get the socket's closing flag.
     * @return If the socket is in closing status.
     */
    bool is_closing() const {
        return closing_;
    }

    /**
     * @brief Set the connect handler which will be called when the connection
     *  is established.
     * @param fn The connect handler function.
     * @return client_socket& Return the client socket object reference.
     */
    client_socket &on_connect(connect_handler_t fn);

    /**
     * @brief Set the read handler which will be called when the socket is readable.
     * @param fn The read handler function.
     * @return client_socket& Return the client socket object reference.
     */
    client_socket &on_read(read_handler_t fn);

    /**
     * @brief Set the write handler which will be called when the socket is writable.
     * @param fn The write handler function.
     * @return client_socket& Return the client socket object reference.
     */
    client_socket &on_write(write_handler_t fn);

    /**
     * @brief Set the error handler which will be called when the socket has an error.
     * @param fn The error handler function.
     * @return client_socket& Return the client socket object reference.
     */
    client_socket &on_error(error_handler_t fn);

    /**
     * @brief Set the close handler which will be called when the socket is closed.
     * @param fn The close handler function.
     * @return client_socket& Return the client socket object reference.
     */
    client_socket &on_close(close_handler_t fn);

public:
    /**
     * @brief Read data from the socket.
     * @param buf The buffer to store the data.
     * @param count The buffer's capacity.
     * @return ssize_t Return the number of bytes read, or -1 if an error occurs, 0 if socket was closed.
     */
    ssize_t read(void *buf, size_t count);

    /**
     * @brief Write data to the socket.
     * @param data The data buffer to write.
     * @param len The data buffer length.
     * @param ms The timeout in milliseconds.
     * @return The number of bytes written, or -1 if an error occurs.
     */
    ssize_t write(const void *data, size_t len, int ms = -1);

    /**
     * @brief Get the socket file descriptor.
     * @return Return the socket file descriptor, or invalid_socket
     *  if an error occurs.
     */
    socket_t sock_handle() const;

    /**
     * @brief Get the nio_event object.
     * @return Return the nio_event.
     */
    nio_event& get_event() const {
        return ev_;
    }

private:
    friend class nio_event;

    nio_event &ev_;
    connect_handler_t on_connect_;
    read_handler_t    on_read_;
    write_handler_t   on_write_;
    error_handler_t   on_error_;
    close_handler_t   on_close_;

    NIO_FILE *fe_  = nullptr;
    bool closing_  = false;
    void close();

    static void connect_callback(NIO_EVENT *ev, NIO_FILE *fe);
    static void read_callback(NIO_EVENT *ev, NIO_FILE *fe);
    static void write_callback(NIO_EVENT *ev, NIO_FILE *fe);

    ssize_t flush();

private:
    client_timer *read_timer_  = nullptr;
    client_timer *write_timer_ = nullptr;
    std::string  *buf_         = nullptr;

    friend class client_timer;
    void on_timer(client_timer *timer);
};

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief The client timer class for client socket timer operations,
 *  inherited from event_timer.
 */
class client_timer : public event_timer {
public:
    explicit client_timer(client_socket &cli) : cli_(cli) {}
    ~client_timer() override = default;

    /**
     * @brief Check if the client socket is in waiting status.
     * @return Return true if the client socket is in waiting status.
     */
    bool is_waiting() const {
        return waiting_;
    }

    /**
     * @brief Set the waiting status of the client socket.
     * @param yes The waiting status to set: true or false.
     */
    void set_waiting(bool yes) {
        waiting_ = yes;
    }

    /**
     * @brief Check if the client socket is in connecting status.
     * @return Return true if the client socket is in connecting status.
     */
    bool is_connecting() const {
        return connecting_;
    }

    /**
     * @brief Set the connecting status of the client socket.
     * @param The connecting status to set: true or false.
     */
    void set_connecting(bool yes) {
        connecting_ = yes;
    }

protected:
    /**
     * @brief When the timer expires, the on_timer function is called.
     */
    void on_timer() override {
        cli_.on_timer(this);
    }

private:
    client_socket &cli_;
    bool waiting_    = false;
    bool connecting_ = false;
};

} // namespace
