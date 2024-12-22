#pragma once

#include <iostream>
#include <memory>
#include <stdexcept>

#include "MessageQueue.hpp"

class Connection {
public:
    enum Mode {
        SEND_ONLY,
        RECEIVE_ONLY,
        CLIENT,
        SERVER
    };

    Connection(const std::string& name, Mode mode) : connection_name(name), connection_mode(mode) {
        switch (mode) {
        case SEND_ONLY:
            send_queue = std::make_unique<MessageQueue>(name);
            break;
        case RECEIVE_ONLY:
            receive_queue = std::make_unique<MessageQueue>(name);
            break;
        case CLIENT:
            send_queue = std::make_unique<MessageQueue>(name + "_send");
            receive_queue = std::make_unique<MessageQueue>(name + "_receive");
            break;
        case SERVER:
            send_queue = std::make_unique<MessageQueue>(name + "_receive");
            receive_queue = std::make_unique<MessageQueue>(name + "_send");
            break;
        default:
            throw std::invalid_argument("Invalid mode");
        }
    }

    void send(const std::string& message) {
        if (!send_queue) {
            throw std::runtime_error("Send operation is not supported in the current mode");
        }
        if (!send_queue->push(message)) {
            throw std::runtime_error("Failed to send message: " + send_queue->get_error_message());
        }
    }

    std::string receive() {
        if (!receive_queue) {
            throw std::runtime_error("Receive operation is not supported in the current mode");
        }
        if (receive_queue->empty()) {
            throw std::runtime_error("No messages available to receive");
        }
        return receive_queue->pop();
    }

    bool empty() {
        if (receive_queue) {
            return receive_queue->empty();
        }
        return false;
    }

private:
    std::string connection_name;
    Mode connection_mode;
    std::unique_ptr<MessageQueue> send_queue;
    std::unique_ptr<MessageQueue> receive_queue;
};
