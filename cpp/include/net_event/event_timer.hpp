//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#pragma once

namespace ev {

class net_event;

class event_timer {
public:
	event_timer() = default;
	virtual ~event_timer() = default;

	virtual void on_timer() = 0;

	long long get_expire() const {
		return stamp_;
	}

public:
	friend class net_event;

	void set_expire(long long when);

private:
	long long stamp_ = 0;
};

} // namespace