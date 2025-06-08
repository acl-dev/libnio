#include "stdafx.hpp"
#include "nio/mqtt/mqtt_pubrel.hpp"

namespace nio {

mqtt_pubrel::mqtt_pubrel()
: mqtt_ack(MQTT_PUBREL)
{
	this->get_header().set_header_flags(0x02);
}

mqtt_pubrel::mqtt_pubrel(const mqtt_header& header)
: mqtt_ack(header)
{
	this->get_header().set_header_flags(0x02);
}

mqtt_pubrel::~mqtt_pubrel() {}

} // namespace
