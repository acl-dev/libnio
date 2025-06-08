#pragma once

#include "mqtt_ack.hpp"

namespace nio {

/**
 * mqtt message object for MQTT_PUBREC type.
 */
class mqtt_pubrec : public mqtt_ack {
public:
	/**
	 * constructor for creating MQTT_PUBREC mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_pubrec();

	/**
	 * constructor for creating MQTT_PUBREC mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_pubrec(const mqtt_header& header);

	~mqtt_pubrec();
};

} // namespace
