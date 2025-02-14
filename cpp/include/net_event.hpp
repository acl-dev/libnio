//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#pragma once
#include <map>

struct NET_EVENT;

namespace ev {

class event_timer;

class net_event {
public:
	explicit net_event(int max_size);

	~net_event();

	void wait(int ms);

	void add_timer(event_timer *tm);

	void del_timer(event_timer *tm);

public:
	NET_EVENT *get_event() const {
		return ev_;
	}

private:
	NET_EVENT *ev_;
	long long stamp_ = 0;
	std::multimap<long long, event_timer *> timers_;

	void set_stamp();

	long long get_stamp() const {
		return stamp_;
	}

	void trigger_timers();
};

} // namespace