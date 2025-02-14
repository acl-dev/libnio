//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#pragma once

namespace ev {

class event_timer {
public:
	explicit event_timer(long long ms);
	virtual ~event_timer() = default;

	virtual void on_timer() = 0;

public:
	void set(long long ms);
	long long get_ms() const {
		return ms_;
	}

private:
	long long ms_;
};

} // namespace