//
// Created by shuxin zheng on 2025/2/14.
//

#pragma once
#include "nio_define.hpp"

struct NIO_FILE;

namespace nio {

class nio_event;

/**
 * @brief The event processor class for the socket read and write operations.
 */
class event_proc {
public:
    explicit event_proc(nio_event &ev, socket_t fd);
    explicit event_proc() = default;
    virtual ~event_proc();

    /**
     * @brief Bind the socket file descriptor to the event processor.
     * @param ev The nio event.
     * @param fd The socket file descriptor.
     */
    void bind(nio_event &ev, socket_t fd);

    /**
     * @brief Read data from the socket in async mode, when the read operation
     *  is successful, the on_read() virtual function will be called.
     * @return Return true if the read operation is successful.
     */
    bool read_await();

    /**
     * @brief Write data to the socket in async mode, when the write operation
     *  is successful, the on_write() virtual function will be called.
     * @return Return true if the write operation is successful.
     */
    bool write_await();

    /**
     * @brief Disable the read event.
     */
    void read_disable();

    /**
     * @brief Disable the write event.
     */
    void write_disable();

    /**
     * @brief If the socket given in the constructor is in connecting status,
     *  this function will wait for the connection to be established, when the
     *  connection is established, the on_connect() virtaul function will be called.
     * @return Return true if the connection operation is successful.
     */
    bool connect_await();

    /**
     * @brief Close the socket in async mode.
     */
    void close_await();

public:
    /**
     * @brief Write data to the socket in async mode, when the write operation
     *  can't write all the data at once, the remaining data will be buffered
     *  and written in the next write operation automatically.
     * @param data The data to write.
     * @param len The length of the data.
     * @return ssize_t Return the number of bytes written, or -1 if an error occurs,
     *  or 0 if the data is buffered.
     */
    ssize_t write(const void *data, size_t len);

    /**
     * @brief Get the event object
     * @return Return the event object.
     */
    nio_event *get_event() const {
        return ev_;
    }

    /**
     * @brief Set the closing flag of the socket.
     */
    void set_closing();

    /**
     * @brief Check if the socket is in closing status.
     * @return If the socket is in closing status.
     */
    bool is_closing() const {
        return closing_;
    }

    /**
     * @brief Get the socket file descriptor.
     * @return socket_t Return the socket file descriptor, or invalid_socket
     *  if an error occurs.
     */
    socket_t sock_handle() const;

protected:
    /**
     * @brief The virtual function to be called when the socket is readable.
     */
    virtual void on_read() {}

    /**
     * @brief The virtual function to be called when the socket is writable.
     */
    virtual void on_write() {}

    /**
     * @brief The virtual function to be called when the connection is established.
     * @param ok If the connection is successful.
     */
    virtual void on_connect(bool ok) { (void) ok; }

    /**
     * @brief The virtual function to be called when the socket has an error.
     */
    virtual void on_error() { this->close_await(); }

public:
    /**
     * @brief The virtual function to be called when the socket is closed.
     */
    virtual void on_close() {}

private:
    nio_event*   ev_      = nullptr;
    NIO_FILE*    fe_      = nullptr;
    std::string* buf_     = nullptr;
    bool         closing_ = false;

    ssize_t flush();

    static void read_proc(NIO_EVENT *ev, NIO_FILE *fe);
    static void write_proc(NIO_EVENT *ev, NIO_FILE *fe);
    static void connect_proc(NIO_EVENT *ev, NIO_FILE *fe);
};

} // namespace
