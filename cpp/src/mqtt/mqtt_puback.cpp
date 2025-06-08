#include "stdafx.hpp"
#include "nio/mqtt/mqtt_puback.hpp"

namespace nio {

mqtt_puback::mqtt_puback()
: mqtt_ack(MQTT_PUBACK)
{
}

mqtt_puback::mqtt_puback(const mqtt_header& header)
: mqtt_ack(header)
{
}

mqtt_puback::~mqtt_puback() {}

} // namespace
