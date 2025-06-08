#include "stdafx.hpp"
#include "nio/mqtt/mqtt_disconnect.hpp"

namespace nio {

mqtt_disconnect::mqtt_disconnect()
: mqtt_message(MQTT_DISCONNECT)
{
}

mqtt_disconnect::mqtt_disconnect(const mqtt_header& header)
: mqtt_message(header)
{
}

mqtt_disconnect::~mqtt_disconnect() {}

bool mqtt_disconnect::to_string(std::string& out) {
    mqtt_header& header = this->get_header();
    header.set_remaing_length(0);

    if (!header.build_header(out)) {
        return false;
    }
    return true;
}

} // namespace
