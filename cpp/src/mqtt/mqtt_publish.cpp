#include "stdafx.hpp"
#include "nio/mqtt/mqtt_publish.hpp"

namespace nio {

enum {
    MQTT_STAT_HDR_VAR,
    MQTT_STAT_TOPIC_LEN,
    MQTT_STAT_TOPIC_VAL,
    MQTT_STAT_PKTID,
    MQTT_STAT_PAYLOAD,
};

mqtt_publish::mqtt_publish()
: mqtt_message(MQTT_PUBLISH)
, pkt_id_(0)
, dlen_(0)
, hlen_var_(0)
, payload_len_(0)
, finished_(false)
{
    status_ = MQTT_STAT_HDR_VAR;  // just for update()
}

mqtt_publish::mqtt_publish(const mqtt_header& header)
: mqtt_message(header)
, pkt_id_(0)
, dlen_(0)
, hlen_var_(0)
, finished_(false)
{
    status_      = MQTT_STAT_HDR_VAR;  // just for update()
    payload_len_ = header.get_remaining_length();
}

mqtt_publish::~mqtt_publish() {}

mqtt_publish& mqtt_publish::set_topic(const char* topic) {
    topic_ = topic;
    return *this;
}

mqtt_publish& mqtt_publish::set_pkt_id(unsigned short id) {
    if (id > 0) {
        pkt_id_ = id;
    } else {
        nio_msg_warn("%s(%d): pkt id should > 0, id=%d",
                __FUNCTION__, __LINE__, id);
    }
    return *this;
}

mqtt_publish& mqtt_publish::set_payload(unsigned len, const char* data /* NULL */) {
    payload_len_ = len;
    if (data && payload_len_ > 0) {
        payload_.assign(data, len);
    }
    return *this;
}

bool mqtt_publish::to_string(std::string& out) {
    mqtt_header& header = this->get_header();
    unsigned len = (unsigned) topic_.size() + 2 + payload_len_;
    mqtt_qos_t qos = header.get_qos();

    if (qos > MQTT_QOS0) {
        if (pkt_id_ == 0) {
            nio_msg_error("%s(%d): pkt_id should > 0, pkt_id=%u",
                    __FUNCTION__, __LINE__, pkt_id_);
            return false;
        }
        len += 2;
    }

    if (header.is_dup() && qos == MQTT_QOS0) {
        header.set_dup(false);
    }

    header.set_remaing_length(len);

    if (!header.build_header(out)) {
        nio_msg_error("%s(%d): build header error", __FUNCTION__, __LINE__);
        return false;
    }

    this->pack_add(topic_, out);

    if (qos != MQTT_QOS0) {
        this->pack_add((unsigned short) pkt_id_, out);
    }

    if (payload_len_ > 0 && !payload_.empty()) {
        out.append(payload_.c_str(), payload_len_);
    }

    return true;
}

static struct {
    int status;
    int (mqtt_publish::*handler)(const char*, int);
} handlers[] = {
    { MQTT_STAT_HDR_VAR,	&mqtt_publish::update_header_var },

    { MQTT_STAT_TOPIC_LEN,	&mqtt_publish::update_topic_len  },
    { MQTT_STAT_TOPIC_VAL,	&mqtt_publish::update_topic_val  },
    { MQTT_STAT_PKTID,	&mqtt_publish::update_pktid      },
    { MQTT_STAT_PAYLOAD,	&mqtt_publish::update_payload    },
};

int mqtt_publish::update(const char* data, int dlen) {
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

int mqtt_publish::update_header_var(const char* data, int dlen) {
    (void) data;
    assert(data && dlen > 0);

    dlen_   = 0;
    status_ = MQTT_STAT_TOPIC_LEN;
    return dlen;
}

#define	HDR_LEN_LEN	2

int mqtt_publish::update_topic_len(const char* data, int dlen) {
    assert(data && dlen > 0);
    assert(sizeof(buff_) >= HDR_LEN_LEN);

    for (; dlen_ < HDR_LEN_LEN && dlen > 0;) {
        buff_[dlen_++] = *data++;
        dlen--;
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
        nio_msg_error("%s(%d): invalid topic len=%d", __FUNCTION__, __LINE__, n);
        return -1;
    }

    dlen_     = n;
    hlen_var_ = n + HDR_LEN_LEN;
    status_   = MQTT_STAT_TOPIC_VAL;

    return dlen;
}

int mqtt_publish::update_topic_val(const char* data, int dlen) {
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

    dlen_   = 0;
    if (this->get_header().get_qos() != MQTT_QOS0) {
        status_ = MQTT_STAT_PKTID;
    } else {
        payload_len_ -= hlen_var_;
        status_ = MQTT_STAT_PAYLOAD;
    }

    return dlen;
}

#define	HDR_PKTID_LEN	2

int mqtt_publish::update_pktid(const char* data, int dlen) {
    assert(data && dlen > 0);
    assert(sizeof(buff_) >= HDR_PKTID_LEN);

    if (dlen_ >= HDR_PKTID_LEN) {
        nio_msg_error("%s(%d): invalid pkt id", __FUNCTION__, __LINE__);
        return -1;
    }

    for (; dlen_ < HDR_PKTID_LEN && dlen > 0;) {
        buff_[dlen_++] = *data++;
        dlen--;
    }

    if (dlen_ < HDR_PKTID_LEN) {
        assert(dlen == 0);
        return dlen;
    }

    if (!this->unpack_short(&buff_[0], 2, pkt_id_)) {
        nio_msg_error("%s(%d): unpack pkt_id error", __FUNCTION__, __LINE__);
        return -1;
    }

    hlen_var_ += HDR_PKTID_LEN;

    if (payload_len_ == 0) {
        finished_ = true;
        return dlen;
    }

    if (payload_len_ < hlen_var_) {
        nio_msg_error("%s(%d): invalid payload len=%u, hlen_var=%u",
                __FUNCTION__, __LINE__, payload_len_, hlen_var_);
        return -1;
    }

    payload_len_ -= hlen_var_;
    if (payload_len_ == 0) {
        finished_ = true;
        return dlen;
    }

    status_ = MQTT_STAT_PAYLOAD;
    return dlen;
}

int mqtt_publish::update_payload(const char* data, int dlen) {
    if ((size_t) payload_len_ <= payload_.size()) {
        nio_msg_error("%s(%d): finished, payload_'s size=%zd, payload_len_=%d",
                __FUNCTION__, __LINE__, payload_.size(), payload_len_);
        return -1;
    }

    assert(data && dlen > 0);

    size_t i, left = (size_t) payload_len_ - payload_.size();
    for (i = 0; i < left && dlen > 0; i++) {
        //payload_.append(data, 1);
        payload_.push_back(*data);
        data++;
        dlen--;
    }

    if (i == left) {
        finished_ = true;
    }

    return dlen;
}

} // namespace
