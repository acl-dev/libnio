#pragma once

#include "mqtt_ack.hpp"

namespace nio {

/**
 * mqtt message object for MQTT_UNPUBACK type.
 */
class mqtt_unsuback : public mqtt_ack {
public:
	/**
	 * constructor for creating MQTT_UNPUBACK mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_unsuback();

	/**
	 * constructor for creating MQTT_UNPUBACK mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_unsuback(const mqtt_header& header);

	~mqtt_unsuback();
};

} // namespace
