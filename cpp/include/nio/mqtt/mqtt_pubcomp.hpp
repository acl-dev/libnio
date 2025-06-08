#pragma once

#include "mqtt_ack.hpp"

namespace nio {

/**
 * mqtt message object for MQTT_PUBCOMP type.
 */
class mqtt_pubcomp : public mqtt_ack {
public:
	/**
	 * constructor for creating MQTT_PUBCOMP mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_pubcomp();

	/**
	 * constructor for creating MQTT_PUBCOMP mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_pubcomp(const mqtt_header& header);

	~mqtt_pubcomp();
};

} // namespace
