//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#include "stdafx.hpp"
#include "event_timer.hpp"
#include "event_proc.hpp"
#include "net_event.hpp"
#include "../../c/include/net_event/net_iostuff.h"

namespace nev {

net_event::net_event(int size, net_event_t type) {
    int et = NET_EVENT_TYPE_KERNEL;
    switch (type) {
    case NET_EVENT_T_POLL:
        et = NET_EVENT_TYPE_POLL;
        break;
    case NET_EVENT_T_SELECT:
        et = NET_EVENT_TYPE_SELECT;
        break;
    case NET_EVENT_T_WMSG:
        et = NET_EVENT_TYPE_WMSG;
        break;
    case NET_EVENT_T_KERNEL:
    default:
        break;
    }
    ev_ = net_event_create(size, et);
    set_stamp();
}

net_event::~net_event() {
    net_event_free(ev_);
}

void net_event::add_timer(event_timer *tm, long long ms) {
    long long when = stamp_ + ms * 1000 + counter_++ % 1000;
    tm->set_expire(when);

    timers_.insert({when, tm});
}

void net_event::del_timer(event_timer *tm) {
    auto tmers = timers_.equal_range(tm->get_expire());
    for (auto it = tmers.first; it != tmers.second; ++it) {
        if (it->second == tm) {
            timers_.erase(it);
            break;
        }
    }
}

void net_event::reset_timer(event_timer *tm, long long ms) {
    del_timer(tm);
    add_timer(tm, ms);
}

void net_event::delay_close(event_proc *proc) {
    if (!proc->is_closing()) {
        procs_free_.push_back(proc);
    }
}

void net_event::before_wait(void *ctx) {
    auto *me = (net_event *) ctx;

    for (auto proc : me->procs_free_) {
        proc->on_close();
    }
    me->procs_free_.clear();
}

void net_event::wait(int ms) {
    auto it = timers_.begin();
    if (it != timers_.end()) {
        int delay = (int) (it->first - stamp_);
        if (delay > 0 && delay < ms) {
            ms = delay;
        }
    }

    net_event_wait2(ev_, ms, before_wait, this);
    set_stamp();
    if (!timers_.empty()) {
        trigger_timers();
    }
}

void net_event::trigger_timers() {
    std::vector<event_timer *> timers;
    for (auto it = timers_.begin(); it != timers_.end();) {
        if (it->first > stamp_) {
            break;
        }
        timers.push_back(it->second);
        it = timers_.erase(it);
    }

    for (auto timer: timers) {
        timer->on_timer();
    }
}

void net_event::set_stamp() {
    struct timeval tm;
    gettimeofday(&tm, nullptr);

    stamp_ = tm.tv_sec * 1000000 + tm.tv_usec;
}

void net_event::set_nblock(int fd, bool yes) {
    net_non_blocking(fd, yes ? 1 : 0);
}

void net_event::set_ndelay(int fd, bool yes) {
    net_tcp_nodelay(fd, yes ? 1 : 0);
}

void net_event::debug(bool on) {
    net_event_debug(on ? 1 : 0);
}

} // namespace