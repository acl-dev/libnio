//
// Created by shuxin zheng on 2025/2/14.
//

#pragma once
#include <map>
#include <list>

struct NIO_EVENT;

namespace nio {

/**
 * @brief The event type can be one of the following:
 */
typedef enum {
    NIO_EVENT_T_POLL,
    NIO_EVENT_T_SELECT,
    NIO_EVENT_T_KERNEL,
    NIO_EVENT_T_WMSG,
} nio_event_t;

class event_timer;
class event_proc;
class client_socket;

/**
 * @brief The nio event class for nio event management.
 */
class nio_event {
public:
    explicit nio_event(int max_size, nio_event_t type = NIO_EVENT_T_KERNEL);

    ~nio_event();

    /**
     * @brief Wait for IO events of all registered file descriptors.
     * @param ms The timeout in milliseconds.
     */
    void wait(int ms);

    /**
     * @brief Add a timer to the event loop.
     * @param tm The timer to add.
     * @param ms The timeout in milliseconds for the timer.
     */
    void add_timer(event_timer *tm, long long ms);

    /**
     * @brief Delete a timer from the event loop.
     * @param tm The timer to delete.
     */
    void del_timer(event_timer *tm);

    /**
     * @brief Reset a timer with a new timeout.
     * @param tm The timer to reset.
     * @param ms The new timeout in milliseconds.
     */
    void reset_timer(event_timer *tm, long long ms);

    /**
     * @brief Delay close a event proc.
     * @param proc The event proc to delay close.
     */
    void delay_close(event_proc *proc);

    /**
     * @brief Delay close a client socket.
     * @param client The client socket to delay close.
     */
    void delay_close(client_socket *client);

public:
    /**
     * @brief Get the C event object
     * @return NIO_EVENT* 
     */
    NIO_EVENT *get_event() const {
        return ev_;
    }

    /**
     * @brief Set the debug flag.
     * @param on If true, the debug flag is set.
     */
    static void debug(bool on);

    /**
     * @brief Set the non-blocking flag of a file descriptor.
     * @param fd The file descriptor.
     * @param yes If true, the non-blocking flag is set.
     */
    static void set_nblock(int fd, bool yes);

    /**
     * @brief Set TCP_NODELAY flag of a file descriptor.
     * @param fd The file descriptor.
     * @param yes If true, the TCP_NODELAY flag is set.
     */
    static void set_ndelay(int fd, bool yes);

private:
    NIO_EVENT *ev_;
    std::list<event_proc*> procs_free_;
    std::list<client_socket*> clients_free_;
    long long stamp_ = 0;
    unsigned counter_ = 0;
    std::multimap<long long, event_timer *> timers_;

    void set_stamp();
    void trigger_timers();

    static void before_wait(void *ctx);
};

} // namespace
