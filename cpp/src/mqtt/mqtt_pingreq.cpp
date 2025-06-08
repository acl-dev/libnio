#include "stdafx.hpp"
#include "nio/mqtt/mqtt_pingreq.hpp"

namespace nio {

mqtt_pingreq::mqtt_pingreq()
: mqtt_message(MQTT_PINGREQ)
{
}

mqtt_pingreq::mqtt_pingreq(const mqtt_header& header)
: mqtt_message(header)
{
}

mqtt_pingreq::~mqtt_pingreq() {}

bool mqtt_pingreq::to_string(std::string& out) {
    mqtt_header& header = this->get_header();

    header.set_remaing_length(0);

    if (!header.build_header(out)) {
        return false;
    }
    return true;
}

} // namespace
