#include "stdafx.hpp"
#include "nio/mqtt/mqtt_subscribe.hpp"

namespace nio {

enum {
    MQTT_STAT_HDR_VAR,
    MQTT_STAT_TOPIC_LEN,
    MQTT_STAT_TOPIC_VAL,
    MQTT_STAT_TOPIC_QOS,
};

mqtt_subscribe::mqtt_subscribe()
: mqtt_message(MQTT_SUBSCRIBE)
, pkt_id_(0)
, dlen_(0)
, body_len_(0)
, nread_(0)
, finished_(false)
{
    status_ = MQTT_STAT_HDR_VAR; /* just for update */
}

mqtt_subscribe::mqtt_subscribe(const mqtt_header& header)
: mqtt_message(MQTT_SUBSCRIBE)
, pkt_id_(0)
, dlen_(0)
, nread_(0)
, finished_(false)
{
    status_   = MQTT_STAT_HDR_VAR; /* just for update */
    body_len_ = header.get_remaining_length();
}

mqtt_subscribe::~mqtt_subscribe() {}

mqtt_subscribe& mqtt_subscribe::set_pkt_id(unsigned short id) {
    if (id > 0) {
        pkt_id_ = id;
    } else {
        nio_msg_error("%s(%d): ivnalid pkt id=%u", __FUNCTION__, __LINE__, id);
    }
    return *this;
}

mqtt_subscribe& mqtt_subscribe::add_topic(const char* topic, mqtt_qos_t qos) {
    topics_.push_back(topic);
    qoses_.push_back(qos);
    body_len_ += 2 + (unsigned) strlen(topic) + 1;
    return *this;
}

bool mqtt_subscribe::to_string(std::string& out) {
    if (topics_.empty()) {
        nio_msg_error("%s(%d): no topic available!", __FUNCTION__, __LINE__);
        return false;
    }

    body_len_ += sizeof(pkt_id_);

    mqtt_header& header = this->get_header();

    header.set_header_flags(0x02)
        .set_remaing_length(body_len_);

    if (!header.build_header(out)) {
        return false;
    }

    this->pack_add((unsigned short) pkt_id_, out);

    size_t n = topics_.size();
    for (size_t i = 0; i < n; i++) {
        this->pack_add(topics_[i], out);
        this->pack_add((unsigned char) qoses_[i], out);
    }

    return true;
}

static struct {
    int status;
    int (mqtt_subscribe::*handler)(const char*, int);
} handlers[] = {
    { MQTT_STAT_HDR_VAR,	&mqtt_subscribe::update_header_var	},

    { MQTT_STAT_TOPIC_LEN,	&mqtt_subscribe::update_topic_len	},
    { MQTT_STAT_TOPIC_VAL,	&mqtt_subscribe::update_topic_val	},
    { MQTT_STAT_TOPIC_QOS,	&mqtt_subscribe::update_topic_qos	},
};

int mqtt_subscribe::update(const char* data, int dlen) {
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

int mqtt_subscribe::update_header_var(const char* data, int dlen) {
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
        nio_msg_error("%s(%d): no payload!", __FUNCTION__, __LINE__);
        return -1;
    }

    dlen_   = 0;
    status_ = MQTT_STAT_TOPIC_LEN;

    return dlen;
}

#define	HDR_LEN_LEN	2

int mqtt_subscribe::update_topic_len(const char* data, int dlen) {
    assert(data && dlen > 0);
    assert(sizeof(buff_) >= HDR_LEN_LEN);

    for (; dlen_ < HDR_LEN_LEN && dlen > 0;) {
        buff_[dlen_++] = *data++;
        dlen--;
        nread_++;
    }

    if (dlen_ < HDR_LEN_LEN) {
        assert(dlen == 0);
        return dlen;
    }

    unsigned short n;
    if (!this->unpack_short(&buff_[0], 2, n)) {
        nio_msg_error("%s(%d): unpack cid len error", __FUNCTION__, __LINE__);
        return -1;
    }

    if (n == 0) {
        nio_msg_error("%s(%d): invalid topic len", __FUNCTION__, __LINE__);
        return -1;
    }

    if (nread_ >= body_len_) {
        nio_msg_error("%s(%d): overflow, nread=%u, payload_len=%u",
                __FUNCTION__, __LINE__, nread_, body_len_);
        return -1;
    }

    dlen_   = n;
    status_ = MQTT_STAT_TOPIC_VAL;

    return dlen;
}

int mqtt_subscribe::update_topic_val(const char* data, int dlen) {
    assert(data && dlen > 0 && dlen_ > 0);

    for (; dlen_ > 0 && dlen > 0;) {
        topic_ += *data++;
        --dlen_;
        --dlen;
    }

    if (dlen_ > 0) {
        assert(dlen == 0);
        return dlen;
    } 

    nread_ += (unsigned) topic_.size();
    dlen_   = 0;
    status_ = MQTT_STAT_TOPIC_QOS;

    return dlen;
}

int mqtt_subscribe::update_topic_qos(const char* data, int dlen) {
    assert(data && dlen > 0);

    if (topic_.empty()) {
        nio_msg_error("%s(%d): no topic got", __FUNCTION__, __LINE__);
        return -1;
    }

    int qos = *data++;
    dlen--;
    nread_++;

    if (qos < MQTT_QOS0 || qos > 0x80 || qos > MQTT_QOS2) {
        nio_msg_warn("%s(%d): invalid qos=%d, topic=%s",
                __FUNCTION__, __LINE__, qos, topic_.c_str());
        qos = MQTT_QOS0;
    }

    topics_.push_back(topic_);
    topic_.clear();

    qoses_.push_back((mqtt_qos_t) qos);

    if (nread_ >= body_len_) {
        finished_ = true;
        return dlen;
    }

    dlen_   = 0;
    status_ = MQTT_STAT_TOPIC_LEN;

    return dlen;
}

} // namespace
