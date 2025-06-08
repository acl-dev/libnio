#include "stdafx.hpp"
#include "nio/mqtt/mqtt_connect.hpp"
#include "nio/mqtt/mqtt_connack.hpp"
#include "nio/mqtt/mqtt_publish.hpp"
#include "nio/mqtt/mqtt_puback.hpp"
#include "nio/mqtt/mqtt_pubrec.hpp"
#include "nio/mqtt/mqtt_pubrel.hpp"
#include "nio/mqtt/mqtt_pubcomp.hpp"
#include "nio/mqtt/mqtt_subscribe.hpp"
#include "nio/mqtt/mqtt_suback.hpp"
#include "nio/mqtt/mqtt_unsubscribe.hpp"
#include "nio/mqtt/mqtt_unsuback.hpp"
#include "nio/mqtt/mqtt_pingreq.hpp"
#include "nio/mqtt/mqtt_pingresp.hpp"
#include "nio/mqtt/mqtt_disconnect.hpp"

#include "nio/mqtt/mqtt_message.hpp"

namespace nio {

mqtt_message::mqtt_message(mqtt_type_t type)
: header_(type)
{
	assert(type >= MQTT_RESERVED_MIN && type < MQTT_RESERVED_MAX);
}

mqtt_message::mqtt_message(const mqtt_header& header)
: header_(header)
{
}

mqtt_message::~mqtt_message(void) {}

void mqtt_message::pack_add(unsigned char ch, std::string& out) {
    out.push_back(ch);
}

//#define MOSQ_MSB(x) (unsigned char)((x & 0xff00) >> 8)
//#define MOSQ_LSB(x) (unsigned char)(x & 0x00ff)

#define	MOSQ_MSB(x) (unsigned char) ((x >> 8) & 0xff)
#define MOSQ_LSB(x) (unsigned char) (x & 0xff)

void mqtt_message::pack_add(unsigned short n, std::string& out) {
    unsigned char ch = MOSQ_MSB(n);
    out.push_back(ch);

    ch = MOSQ_LSB(n);
    out.push_back(ch);
}

void mqtt_message::pack_add(const std::string& s, std::string& out) {
    unsigned short n = (unsigned short) s.size();

    unsigned char ch = MOSQ_MSB(n);
    out.push_back(ch);

    ch = MOSQ_LSB(n);
    out.push_back(ch);

    if (n > 0) {
        out.append(s.c_str(), s.size());
    }
}

bool mqtt_message::unpack_short(const char* in, size_t len, unsigned short& out) {
    if (len < 2) {
        nio_msg_error("%s(%d): too short: %ld", __FUNCTION__, __LINE__, (long) len);
        return false;
    }

    out = (unsigned short) (((unsigned short) (in[0] & 0xff) << 8)
            | (unsigned short) (in[1] & 0xff));
    return true;
}

mqtt_message* mqtt_message::create_message(const mqtt_header& header) {
    mqtt_type_t type = header.get_type();
    mqtt_message* message;

    switch (type) {
    case MQTT_CONNECT:
        message = NEW mqtt_connect(header);
        break;
    case MQTT_CONNACK:
        message = NEW mqtt_connack(header);
        break;
    case MQTT_PUBLISH:
        message = NEW mqtt_publish(header);
        break;
    case MQTT_PUBACK:
        message = NEW mqtt_puback(header);
        break;
    case MQTT_PUBREC:
        message = NEW mqtt_pubrec(header);
        break;
    case MQTT_PUBREL:
        message = NEW mqtt_pubrel(header);
        break;
    case MQTT_PUBCOMP:
        message = NEW mqtt_pubcomp(header);
        break;
    case MQTT_SUBSCRIBE:
        message = NEW mqtt_subscribe(header);
        break;
    case MQTT_SUBACK:
        message = NEW mqtt_suback(header);
        break;
    case MQTT_UNSUBSCRIBE:
        message = NEW mqtt_unsubscribe(header);
        break;
    case MQTT_UNSUBACK:
        message = NEW mqtt_unsuback(header);
        break;
    case MQTT_PINGREQ:
        message = NEW mqtt_pingreq(header);
        break;
    case MQTT_PINGRESP:
        message = NEW mqtt_pingresp(header);
        break;
    case MQTT_DISCONNECT:
        message = NEW mqtt_disconnect(header);
        break;
    default:
        nio_msg_error("%s(%d): unknown mqtt type=%d",
                __FUNCTION__, __LINE__, (int) type);
            message = NULL;
            break;
    }
    return message;
}

} //namespace
