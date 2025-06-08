#include "stdafx.hpp"
#include "nio/mqtt/mqtt_unsuback.hpp"

namespace nio {

mqtt_unsuback::mqtt_unsuback()
: mqtt_ack(MQTT_UNSUBACK)
{
}

mqtt_unsuback::mqtt_unsuback(const mqtt_header& header)
: mqtt_ack(header)
{
}

mqtt_unsuback::~mqtt_unsuback() {}

} // namespace
