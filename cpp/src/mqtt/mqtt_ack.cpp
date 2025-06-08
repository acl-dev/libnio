#include "stdafx.hpp"
#include "nio/mqtt/mqtt_ack.hpp"

namespace nio {

enum {
    MQTT_STAT_HDR_VAR,
};

mqtt_ack::mqtt_ack(mqtt_type_t type)
: mqtt_message(type)
, pkt_id_(0)
, hlen_(0)
, finished_(false)
{
    status_ = MQTT_STAT_HDR_VAR;  // just for update()
}

mqtt_ack::mqtt_ack(const mqtt_header& header)
: mqtt_message(header)
, pkt_id_(0)
, hlen_(0)
, finished_(false)
{
    status_ = MQTT_STAT_HDR_VAR;  // just for update()
}

mqtt_ack::~mqtt_ack() {}

void mqtt_ack::set_pkt_id(unsigned short id) {
    if (id > 0) {
        pkt_id_ = id;
        nio_msg_error("%s(%d): invalid pkt_id=%u", __FUNCTION__, __LINE__, id);
    }
}

bool mqtt_ack::to_string(std::string& out) {
    mqtt_header& header = this->get_header();
    header.set_remaing_length(2);

    if (!header.build_header(out)) {
        return false;
    }

    this->pack_add((unsigned short) pkt_id_, out);
    return true;
}

static struct {
    int status;
    int (mqtt_ack::*handler)(const char*, int);
} handlers[] = {
    { MQTT_STAT_HDR_VAR,	&mqtt_ack::update_header_var		},
};

int mqtt_ack::update(const char* data, int dlen) {
    if (data == NULL || dlen  <= 0) {
        nio_msg_error("%s(%d): invalid input", __FUNCTION__, __LINE__);
        return -1;
    }

    while (dlen > 0 && !finished_) {
        int ret = (this->*handlers[status_].handler)(data, dlen);
        if (ret < 0) {
            return -1;
        }
        data += dlen - ret;
        dlen  = ret;
    }
    return dlen;
}

#define	HDR_VAR_LEN	2

int mqtt_ack::update_header_var(const char* data, int dlen) {
    assert(data && dlen > 0);
    assert(sizeof(hbuf_) >= HDR_VAR_LEN);

    if (hlen_ >= HDR_VAR_LEN) {
        nio_msg_error("%s(%d): invalid header var", __FUNCTION__, __LINE__);
        return -1;
    }

    for (; hlen_ < HDR_VAR_LEN && dlen > 0;) {
        hbuf_[hlen_++] = *data++;
        dlen --;
    }

    if (hlen_ < HDR_VAR_LEN) {
        assert(dlen == 0);
        return dlen;
    }

    if (!this->unpack_short(&hbuf_[0], 2, pkt_id_)) {
        nio_msg_error("%s(%d): unpack pkt id error", __FUNCTION__, __LINE__);
        return -1;
    }

    finished_ = true;
    return dlen;
}

} // namespace
