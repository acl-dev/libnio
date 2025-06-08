#pragma once

#include "mqtt_message.hpp"

namespace nio {

/**
 * mqtt message object for MQTT_PINGRESP type.
 */
class mqtt_pingresp : public mqtt_message {
public:
	/**
	 * constructor for creating MQTT_PINGRESP mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_pingresp();

	/**
	 * constructor for creating MQTT_PINGRESP mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_pingresp(const mqtt_header& header);

	~mqtt_pingresp();

protected:
	// @override
	bool to_string(std::string& out);

	// @override
	int update(const char*, int dlen) {
		return dlen;
	}
	// @override
	bool finished() const {
		return true;
	}
};

} // namespace
