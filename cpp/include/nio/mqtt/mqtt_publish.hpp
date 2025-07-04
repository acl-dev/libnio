#pragma once

#include <string>
#include "mqtt_message.hpp"

namespace nio {

/**
 * mqtt message object for MQTT_PUBLISH type.
 */
class mqtt_publish : public mqtt_message {
public:
	/**
	 * constructor for creating MQTT_PUBLISH mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_publish();

	/**
	 * constructor for creating MQTT_PUBLISH mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_publish(const mqtt_header& header);

	~mqtt_publish();

	/**
	 * set the message topic.
	 * @param topic {const char*}
	 * @return {mqtt_publish&}
	 */
	mqtt_publish& set_topic(const char* topic);

	/**
	 * set the message id.
	 * @param id {unsigned short} the value must > 0 && <= 65535.
	 * @return {mqtt_publish&}
	 */
	mqtt_publish& set_pkt_id(unsigned short id);

	/**
	 * set the message payload.
	 * @param len {unsigned} the length of the payload.
	 * @param data {const char*} the payload data.
	 * @return {mqtt_publish&}
	 */
	mqtt_publish& set_payload(unsigned len, const char* data = NULL);

	/**
	 * get the message's topic.
	 * @return {const char*}
	 */
	const char* get_topic() const {
		return topic_.empty() ? "" : topic_.c_str();
	}

	/**
	 * get the message's id.
	 * @return {unsigned short} the message will be invalid if return 0.
	 */
	unsigned short get_pkt_id() const {
		return pkt_id_;
	}

	/**
	 * get the length of the payload.
	 * @return {unsigned}
	 */
	unsigned get_payload_len() const {
		return payload_len_;
	}

	/**
	 * get the palyload.
	 * @return {const string&}
	 */
	const std::string& get_payload() const {
		return payload_;
	}

protected:
	// @override
	bool to_string(std::string& out);

	// @override
	int update(const char* data, int dlen);

	// @override
	bool finished() const {
		return finished_;
	}

public:
	// the below methods were used internal to parse mqtt message
	// in streaming mode.

	int update_header_var(const char* data, int dlen);
	int update_topic_len(const char* data, int dlen);
	int update_topic_val(const char* data, int dlen);
	int update_pktid(const char* data, int dlen);
	int update_payload(const char* data, int dlen);

private:
	unsigned short pkt_id_;
	char buff_[2];
	int  dlen_;
	unsigned status_;
	unsigned hlen_var_;

	std::string topic_;
    std::string payload_;
	unsigned payload_len_;
	bool finished_;
};

} // namespace
