#pragma once
#include <zmq.hpp>
#include <optional>

bool Send(zmq::socket_t& receiver, std::string message) {
    return receiver.send(zmq::buffer(message), zmq::send_flags::dontwait).has_value();
}

std::optional<std::string> Receive(std::optional<zmq::socket_t>& sender) {
    if (sender.has_value()) {
        zmq::message_t request;
        if (sender.value().recv(request, zmq::recv_flags::dontwait).has_value()) {
            return {request.to_string()};
        }
    }
    return std::nullopt;
}