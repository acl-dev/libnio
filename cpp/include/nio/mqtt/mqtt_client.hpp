#pragma once
#include "../client_socket.hpp"

namespace nio {

class mqtt_header;
class mqtt_message;
class mqtt_client;

// Forward declaration of the message handler type.
// This handler will be called when a message is received.
using message_handler_t = std::function<bool(const mqtt_message &msg)>;

// Forward declaration of the timeout handler type.
// This handler will be called when a timeout occurs during read operations.
// It should return true if the timeout was handled successfully, false otherwise.
using timeout_handler_t = std::function<bool(mqtt_client &client)>;

/**
 * @brief The mqtt_client class for handling MQTT client operations.
 */
class mqtt_client {
public:
    explicit mqtt_client(client_socket &conn);
    ~mqtt_client();

    /**
     * @brief Set the message handler which will be called when a message is received.
     * @param fn The message handler function.
     * @return mqtt_client& Return the mqtt client object reference.
     */
    mqtt_client &on_message(message_handler_t fn);

    /**
     * @brief Set the timeout handler which will be called when a timeout occurs.
     * 
     * @param fn The timeout handler function.
     * @return mqtt_client& Return the mqtt client object reference.
     */
    mqtt_client &on_timeout(timeout_handler_t fn);

    /**
     * @brief Read data from the MQTT client in async mode.
     * @param ms The timeout in milliseconds. If -1, it will wait indefinitely.
     * @return If the read operation is successful.
     */
    bool read_await(int ms = -1);

    /**
     * @brief Send a message to the MQTT client and wait for the operation to complete.
     * @param message The MQTT message to send.
     * @return If the message was sent successfully.
     */
    bool send_await(mqtt_message &message);

public:
    /**
     * @brief Get the connection object associated with this MQTT client.
     * @return client_socket& Return the client socket object reference.
     */
    client_socket &get_conn() const {
        return conn_;
    }

private:
    client_socket &conn_;
    message_handler_t message_handler_;
    timeout_handler_t timeout_handler_;

    mqtt_header  *header_  = nullptr;
    mqtt_message *message_ = nullptr;

    ssize_t handle_data(char *data, ssize_t len);
};

} // namespace
