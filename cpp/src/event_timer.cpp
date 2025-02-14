//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#include "stdafx.hpp"
#include "event_timer.hpp"

namespace ev {

event_timer::event_timer(long long ms) : ms_(ms) {}

void event_timer::set(long long int ms) {
	ms_ = ms;
}

} // namespace
