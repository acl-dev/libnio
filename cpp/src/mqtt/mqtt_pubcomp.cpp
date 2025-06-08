#include "stdafx.hpp"
#include "nio/mqtt/mqtt_pubcomp.hpp"

namespace nio {

mqtt_pubcomp::mqtt_pubcomp()
: mqtt_ack(MQTT_PUBCOMP)
{
}

mqtt_pubcomp::mqtt_pubcomp(const mqtt_header& header)
: mqtt_ack(header)
{
}

mqtt_pubcomp::~mqtt_pubcomp() {}

} // namespace
