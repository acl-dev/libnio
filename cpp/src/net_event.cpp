//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#include "stdafx.hpp"
#include "event_timer.hpp"
#include "net_event.hpp"

namespace ev {

net_event::net_event(int size) {
	ev_ = net_event_create(size, NET_EVENT_TYPE_POLL);
	set_stamp();
}

net_event::~net_event() {
	net_event_free(ev_);
}

void net_event::add_timer(event_timer *tm) {
	timers_.insert({stamp_ + tm->get_ms(), tm});
}

void net_event::del_timer(event_timer *tm) {
	auto tmers = timers_.equal_range(tm->get_ms());
	for (auto it = tmers.first; it != tmers.second; ++it) {
		if (it->second == tm) {
			timers_.erase(it);
			break;
		}
	}
}

void net_event::wait(int ms) {
	auto it = timers_.begin();
	if (it != timers_.end()) {
		int delay = (int) (it->first - stamp_);
		if (delay > 0 && delay < ms) {
			ms = delay;
		}
	}

	net_event_wait(ev_, ms);
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

	stamp_ = tm.tv_sec * 1000 + tm.tv_usec / 1000;
}

} // namespace