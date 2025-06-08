#pragma once

#include "mqtt_message.hpp"

namespace nio {

/**
 * mqtt message object for the MQTT_DISCONNECT type.
 */
class mqtt_disconnect : public mqtt_message {
public:
	/**
	 * constructor for creating MQTT_DISCONNECT mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_disconnect();

	/**
	 * constructor for creating MQTT_DISCONNECT mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_disconnect(const mqtt_header& header);

	~mqtt_disconnect();

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
