//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#pragma once
#include <map>
#include <list>

struct NET_EVENT;

namespace ev {

typedef enum {
	NET_EVENT_T_POLL,
	NET_EVENT_T_SELECT,
	NET_EVENT_T_KERNEL,
	NET_EVENT_T_WMSG,
} net_event_t;

class event_timer;
class event_proc;

class net_event {
public:
	explicit net_event(int max_size, net_event_t type = NET_EVENT_T_KERNEL);

	~net_event();

	void wait(int ms);

	void add_timer(event_timer *tm, long long ms);
	void del_timer(event_timer *tm);
	void reset_timer(event_timer *tm, long long ms);

	void delay_destroy(event_proc *proc);

public:
	NET_EVENT *get_event() const {
		return ev_;
	}

private:
	NET_EVENT *ev_;
	std::list<event_proc*> procs_free_;
	long long stamp_ = 0;
	unsigned counter_ = 0;
	std::multimap<long long, event_timer *> timers_;

	void set_stamp();
	void trigger_timers();

	static void before_wait(void *ctx);
};

} // namespace