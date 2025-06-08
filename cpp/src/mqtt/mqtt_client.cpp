#include "stdafx.hpp"
#include "nio/mqtt/mqtt_header.hpp"
#include "nio/mqtt/mqtt_message.hpp"
#include "nio/client_socket.hpp"

#include "nio/mqtt/mqtt_client.hpp"

namespace nio {

mqtt_client::mqtt_client(client_socket &conn)
: conn_(conn)
{
    header_  = new mqtt_header(MQTT_RESERVED_MIN);
}

mqtt_client::~mqtt_client() {
    delete header_;
    delete message_;
}

mqtt_client &mqtt_client::on_message(message_handler_t fn) {
    message_handler_ = std::move(fn);
    return *this;
}

mqtt_client &mqtt_client::on_timeout(timeout_handler_t fn) {
    timeout_handler_ = std::move(fn);
    return *this;
}

bool mqtt_client::read_await(int ms) {
    // This function should implement the logic to read data from the MQTT client.
    // It should call the message_handler_ when a message is received.
    // For now, we will just return true to indicate success.
    // Actual implementation would involve reading from the socket and parsing MQTT messages.

    conn_.on_read([this](socket_t fd, bool expired) {
        if (expired) {
            // If the read operation has expired, we can handle it here.
            if (timeout_handler_ == nullptr || !timeout_handler_(*this)) {
                conn_.close_await();
            }
            return;
        }

        char buf[1024], *data = buf;
        ssize_t dlen = conn_.read(buf, sizeof(buf));
        if (dlen <= 0) {
            // If no bytes were read or an error occurred, we can close the connection.
            conn_.close_await();
            return;
        }

        while (true) {
            ssize_t left = handle_data(data, dlen);
            if (left < 0) {
                conn_.close_await();
                break;
            } else if (left > 0) {
                data += dlen - left;
                dlen = left;
            }  else {
                break;
            }
        } 
    });

    // Call the read_await method of the client_socket to perform the actual read operation.
    return conn_.read_await(ms);
}

ssize_t mqtt_client::handle_data(char *data, ssize_t len) {
    int left;
    if (!header_->finished()) {
        // If the header is not finished, we need to read more data.
        left = header_->update(data, len);
    } else {
        left = len;
    }

    if (left < 0) {
        return -1; // If there was an error updating the header, we return -1.
    }

    if (!header_->finished()) {
        assert(left == 0);
        return 0; // We need more data to complete the header.
    }

    data += len - left;
    len = left;

    if (message_ == nullptr) {
        // If we don't have a message object, we create one based on the header.
        message_ = mqtt_message::create_message(*header_);
        if (message_ == nullptr) {
            // If the message type is invalid, we close the connection.
            return -1;
        }
    }

    if (len > 0) {
        // If we have data to update the message, we do so.
        left = message_->update(data, len);
        if (left < 0) {
            // If there was an error updating the message, we close the connection.
            return -1;
        }
    }

    if (message_->finished()) {
        bool err = false;
        // If the message is finished, we call the message handler.
        if (message_handler_ && !message_handler_(*message_)) {
            err = true; // If the message handler returns false, we set an error flag.
        }
        // Reset the message header for the next read operation.
        header_->reset();

        delete message_;
        message_ = nullptr;
        if (err) {
            return -1;
        }
    }

    return left; // Return the number of bytes left to process.
}

bool mqtt_client::send_await(mqtt_message &message) {
    // This function should implement the logic to send a message to the MQTT client.
    // It should serialize the message and write it to the socket.
    // For now, we will just return true to indicate success.
    // Actual implementation would involve serializing the MQTT message and writing it to the socket.

    std::string out;
    if (!message.to_string(out) || out.empty()) {
        return false; // If serialization fails, we return false.
    }

    conn_.write(out.data(), out.size());
    return true;
}

} // namespace
