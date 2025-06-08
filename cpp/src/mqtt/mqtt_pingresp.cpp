#include "stdafx.hpp"
#include "nio/mqtt/mqtt_pingresp.hpp"

namespace nio {

mqtt_pingresp::mqtt_pingresp()
: mqtt_message(MQTT_PINGRESP)
{
}

mqtt_pingresp::mqtt_pingresp(const mqtt_header& header)
: mqtt_message(header)
{
}

mqtt_pingresp::~mqtt_pingresp() {}

bool mqtt_pingresp::to_string(std::string& out) {
    mqtt_header& header = this->get_header();
    header.set_remaing_length(0);

    if (!header.build_header(out)) {
        return false;
    }
    return true;
}

} // namespace
