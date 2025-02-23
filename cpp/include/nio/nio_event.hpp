//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#pragma once
#include <map>
#include <list>

struct NIO_EVENT;

namespace nio {

typedef enum {
    NIO_EVENT_T_POLL,
    NIO_EVENT_T_SELECT,
    NIO_EVENT_T_KERNEL,
    NIO_EVENT_T_WMSG,
} nio_event_t;

class event_timer;
class event_proc;

class nio_event {
public:
    explicit nio_event(int max_size, nio_event_t type = NIO_EVENT_T_KERNEL);

    ~nio_event();

    void wait(int ms);

    void add_timer(event_timer *tm, long long ms);
    void del_timer(event_timer *tm);
    void reset_timer(event_timer *tm, long long ms);

    void delay_close(event_proc *proc);

public:
    NIO_EVENT *get_event() const {
        return ev_;
    }

    static void debug(bool on);

    static void set_nblock(int fd, bool yes);
    static void set_ndelay(int fd, bool yes);

private:
    NIO_EVENT *ev_;
    std::list<event_proc*> procs_free_;
    long long stamp_ = 0;
    unsigned counter_ = 0;
    std::multimap<long long, event_timer *> timers_;

    void set_stamp();
    void trigger_timers();

    static void before_wait(void *ctx);
};

} // namespace
