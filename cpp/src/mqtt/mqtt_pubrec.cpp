#include "stdafx.hpp"
#include "nio/mqtt/mqtt_pubrec.hpp"

namespace nio {

mqtt_pubrec::mqtt_pubrec()
: mqtt_ack(MQTT_PUBREC)
{
}

mqtt_pubrec::mqtt_pubrec(const mqtt_header& header)
: mqtt_ack(header)
{
}

mqtt_pubrec::~mqtt_pubrec() {}

} // namespace
