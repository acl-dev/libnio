#include "stdafx.hpp"
#include "nio/mqtt/mqtt_suback.hpp"

namespace nio {

enum {
    MQTT_STAT_HDR_VAR,
    MQTT_STAT_TOPIC_QOS,
};

mqtt_suback::mqtt_suback()
: mqtt_message(MQTT_SUBACK)
, pkt_id_(0)
, dlen_(0)
, body_len_(0)
, nread_(0)
, finished_(false)
{
    status_ = MQTT_STAT_HDR_VAR; /* just for update */
}

mqtt_suback::mqtt_suback(const mqtt_header& header)
: mqtt_message(MQTT_SUBACK)
, pkt_id_(0)
, dlen_(0)
, nread_(0)
, finished_(false)
{
    status_   = MQTT_STAT_HDR_VAR; /* just for update */
    body_len_ = header.get_remaining_length();
}

mqtt_suback::~mqtt_suback() {}

mqtt_suback& mqtt_suback::set_pkt_id(unsigned short id) {
    pkt_id_ = id;
    return *this;
}

mqtt_suback& mqtt_suback::add_topic_qos(mqtt_qos_t qos) {
    qoses_.push_back(qos);
    body_len_ += 1;
    return *this;
}

mqtt_suback& mqtt_suback::add_topic_qos(const std::vector<mqtt_qos_t>& qoses) {
    for (std::vector<mqtt_qos_t>::const_iterator cit = qoses.begin();
            cit != qoses.end(); ++cit) {
        add_topic_qos(*cit);
    }
    return *this;
}

bool mqtt_suback::to_string(std::string& out) {
    if (qoses_.empty()) {
        nio_msg_error("%s(%d): no qos available!", __FUNCTION__, __LINE__);
        return false;
    }

    body_len_ += sizeof(pkt_id_);

    mqtt_header& header = this->get_header();
    header.set_remaing_length(body_len_);

    if (!header.build_header(out)) {
        return false;
    }

    this->pack_add((unsigned short) pkt_id_, out);

    for (std::vector<mqtt_qos_t>::const_iterator cit = qoses_.begin();
            cit != qoses_.end(); ++cit) {
        this->pack_add((unsigned char) (*cit), out);
    }

    return true;
}

static struct {
    int status;
    int (mqtt_suback::*handler)(const char*, int);
} handlers[] = {
    { MQTT_STAT_HDR_VAR,	&mqtt_suback::update_header_var		},

    { MQTT_STAT_TOPIC_QOS,	&mqtt_suback::update_topic_qos		},
};

int mqtt_suback::update(const char* data, int dlen) {
    if (data == NULL || dlen <= 0) {
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

int mqtt_suback::update_header_var(const char* data, int dlen) {
    assert(data && dlen > 0);
    assert(sizeof(buff_) >= HDR_VAR_LEN);

    if (nread_ >= HDR_VAR_LEN) {
        nio_msg_error("%s(%d): invalid header var", __FUNCTION__, __LINE__);
        return -1;
    }

    for (; nread_ < HDR_VAR_LEN && dlen > 0;) {
        buff_[nread_++] = *data++;
        dlen--;
    }

    if (nread_ < HDR_VAR_LEN) {
        assert(dlen == 0);
        return dlen;
    }

    if (!this->unpack_short(&buff_[0], 2, pkt_id_)) {
        nio_msg_error("%s(%d): unpack pkt id error", __FUNCTION__, __LINE__);
        return -1;
    }

    if (nread_ >= body_len_) {
        nio_msg_warn("%s(%d): no payload!", __FUNCTION__, __LINE__);
        finished_ = true;
        return dlen;
    }

    dlen_   = 0;
    status_ = MQTT_STAT_TOPIC_QOS;
    return dlen;
}

int mqtt_suback::update_topic_qos(const char* data, int dlen) {
    assert(data && dlen > 0);

    int qos = *data++;
    dlen--;
    nread_++;

    if (qos < MQTT_QOS0 || qos > 0x80 || qos > MQTT_QOS2) {
        nio_msg_warn("%s(%d): invalid qos=%d", __FUNCTION__, __LINE__, qos);
        qos = MQTT_QOS0;
    }

    qoses_.push_back((mqtt_qos_t) qos);

    if (nread_ >= body_len_) {
        finished_ = true;
        return dlen;
    }

    dlen_   = 0;
    status_ = MQTT_STAT_TOPIC_QOS;

    return dlen;
}

} // namespace
