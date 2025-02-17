//
// Created by shuxin ¡¡¡¡zheng on 2025/2/14.
//

#include "stdafx.hpp"
#include "net_event.hpp"
#include "event_proc.hpp"

namespace ev {

event_proc::event_proc(net_event &ev, int fd) : ev_(ev) {
	fe_ = net_file_alloc(fd);
	net_file_set_ctx(fe_,this);
}

event_proc::~event_proc() {
	net_file_free(fe_);
}

void event_proc::read_proc(NET_EVENT *, NET_FILE *fe) {
	auto* me = (event_proc*) net_file_get_ctx(fe);
	assert(me);
	if (!me->read()) {
		me->disable_read();
	}
}

void event_proc::write_proc(NET_EVENT *, NET_FILE *fe) {
	auto* me = (event_proc*) net_file_get_ctx(fe);
	assert(me);
	if (!me->write()) {
		me->disable_write();
	}
}

bool event_proc::read_await() {
	return net_event_add_read(ev_.get_event(), fe_, read_proc) != 0;
}

bool event_proc::write_await() {
	return net_event_add_write(ev_.get_event(), fe_, write_proc) != 0;
}

void event_proc::disable_read() {
	net_event_del_read(ev_.get_event(), fe_);
}

void event_proc::disable_write() {
	net_event_del_write(ev_.get_event(), fe_);
}

} // namespace
