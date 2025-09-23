//
// Created by shuxin zheng on 2025/2/14.
//

#include "stdafx.hpp"
#include "nio/event_timer.hpp"
#include "nio/event_proc.hpp"
#include "nio/client_socket.hpp"
#include "nio/nio_event.hpp"
#include "../../c/include/nio/nio_iostuff.h"

namespace nio {

nio_event::nio_event(const int max_size, const nio_event_t type, const unsigned flags) {
    int et = NIO_EVENT_TYPE_KERNEL;
    switch (type) {
    case NIO_EVENT_T_POLL:
        et = NIO_EVENT_TYPE_POLL;
        break;
    case NIO_EVENT_T_SELECT:
        et = NIO_EVENT_TYPE_SELECT;
        break;
    case NIO_EVENT_T_WMSG:
        et = NIO_EVENT_TYPE_WMSG;
        break;
    case NIO_EVENT_T_KERNEL:
    default:
        break;
    }

    unsigned f = 0;
    if ((flags & NIO_EVENT_F_DIRECT) != 0) {
        f |= EVENT_F_DIRECT;
    }

    ev_ = nio_event_create(max_size, et, f);
}

nio_event::~nio_event() {
    nio_event_free(ev_);
}

//#define SEC2NS  1000000000
#define MS2NS   1000000
//#define MS2US   1000
//#define US2NS   1000

void nio_event::add_timer(event_timer *tm, const long long ms) {
    const long long stamp = nio_event_stamp(ev_);
    long long when = stamp + ms * MS2NS + counter_ % MS2NS;
    tm->set_expire(when);
    timers_.insert({when, tm});
}

void nio_event::del_timer(const event_timer *tm) {
    const auto timers = timers_.equal_range(tm->get_expire());
    for (auto it = timers.first; it != timers.second; ++it) {
        if (it->second == tm) {
            timers_.erase(it);
            break;
        }
    }
}

void nio_event::reset_timer(event_timer *tm, const long long ms) {
    del_timer(tm);
    add_timer(tm, ms);
}

void nio_event::delay_close(event_proc *proc) {
    if (!proc->is_closing()) {
        proc->set_closing();
        procs_free_.push_back(proc);
    }
}

void nio_event::delay_close(client_socket *client) {
    if (!client->is_closing()) {
        client->set_closing();
        clients_free_.push_back(client);
    }
}

void nio_event::before_wait(void *ctx) {
    auto *me = static_cast<nio_event*>(ctx);

    for (const auto proc : me->procs_free_) {
        proc->on_close();
    }
    me->procs_free_.clear();

    for (const auto client : me->clients_free_) {
        client->close();
    }
    me->clients_free_.clear();
}

void nio_event::wait(int ms) {
    const auto it = timers_.begin();
    if (it != timers_.end()) {
        const long long stamp = nio_event_stamp(ev_);
        const int delay = static_cast<int>(it->first - stamp) / MS2NS;
        if (delay > 0 && delay < ms) {
            ms = delay;
        }
    }

    nio_event_wait2(ev_, ms, before_wait, this);

    if (!timers_.empty()) {
        trigger_timers();
    }
}

void nio_event::trigger_timers() {
    const long long stamp = nio_event_stamp(ev_);
    std::vector<event_timer *> timers;
    for (auto it = timers_.begin(); it != timers_.end();) {
        if (it->first > stamp) {
            break;
        }
        timers.push_back(it->second);
        it = timers_.erase(it);
    }

    for (const auto timer: timers) {
        timer->on_timer();
    }
}

void nio_event::set_nblock(const int fd, const bool yes) {
    nio_non_blocking(fd, yes ? 1 : 0);
}

void nio_event::set_ndelay(const int fd, const bool yes) {
    nio_tcp_nodelay(fd, yes ? 1 : 0);
}

void nio_event::debug(const bool on) {
    nio_event_debug(on ? 1 : 0);
}

} // namespace
