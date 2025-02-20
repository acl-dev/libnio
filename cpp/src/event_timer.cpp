//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#include "stdafx.hpp"
#include "event_timer.hpp"

namespace nev {

void event_timer::set_expire(long long int when) {
	stamp_ = when;
}

} // namespace
