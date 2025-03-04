//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#include "stdafx.hpp"
#include "nio/event_timer.hpp"
#include "nio/event_proc.hpp"
#include "nio/nio_event.hpp"
#include "../../c/include/nio/nio_iostuff.h"

namespace nio {

nio_event::nio_event(int size, nio_event_t type) {
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
    ev_ = nio_event_create(size, et);
    set_stamp();
}

nio_event::~nio_event() {
    nio_event_free(ev_);
}

void nio_event::add_timer(event_timer *tm, long long ms) {
    long long when = stamp_ + ms * 1000 + counter_++ % 1000;
    tm->set_expire(when);

    timers_.insert({when, tm});
}

void nio_event::del_timer(event_timer *tm) {
    auto tmers = timers_.equal_range(tm->get_expire());
    for (auto it = tmers.first; it != tmers.second; ++it) {
        if (it->second == tm) {
            timers_.erase(it);
            break;
        }
    }
}

void nio_event::reset_timer(event_timer *tm, long long ms) {
    del_timer(tm);
    add_timer(tm, ms);
}

void nio_event::delay_close(event_proc *proc) {
    if (!proc->is_closing()) {
        procs_free_.push_back(proc);
    }
}

void nio_event::before_wait(void *ctx) {
    auto *me = (nio_event *) ctx;

    for (auto proc : me->procs_free_) {
        proc->on_close();
    }
    me->procs_free_.clear();
}

void nio_event::wait(int ms) {
    auto it = timers_.begin();
    if (it != timers_.end()) {
        int delay = (int) (it->first - stamp_);
        if (delay > 0 && delay < ms) {
            ms = delay;
        }
    }

    nio_event_wait2(ev_, ms, before_wait, this);
    set_stamp();
    if (!timers_.empty()) {
        trigger_timers();
    }
}

void nio_event::trigger_timers() {
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

void nio_event::set_stamp() {
    struct timeval tm { 0, 0 };
    gettimeofday(&tm, nullptr);

    stamp_ = tm.tv_sec * 1000000 + tm.tv_usec;
}

void nio_event::set_nblock(int fd, bool yes) {
    nio_non_blocking(fd, yes ? 1 : 0);
}

void nio_event::set_ndelay(int fd, bool yes) {
    nio_tcp_nodelay(fd, yes ? 1 : 0);
}

void nio_event::debug(bool on) {
    nio_event_debug(on ? 1 : 0);
}

} // namespace
