#pragma once

#include "mqtt_ack.hpp"

namespace nio {

/**
 * mqtt message object for MQTT_PUBACK type.
 */
class mqtt_puback : public mqtt_ack {
public:
	/**
	 * constructor for creating MQTT_PUBACK mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_puback();

	/**
	 * constructor for creating MQTT_PUBACK mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_puback(const mqtt_header& header);

	~mqtt_puback();
};

} // namespace
