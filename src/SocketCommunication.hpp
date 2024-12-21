#pragma once
#include <zmq.hpp>
#include <optional>

bool Send(zmq::socket_t& receiver, std::string message) {
    zmq::message_t zmes(message.size());
    memcpy(zmes.data(), message.c_str(), message.size());
    int attempt = 5;
    bool result;
    do {
        result = receiver.send(zmes, zmq::send_flags::none).has_value();
    } while (!result && --attempt);

    std::cout << "    send{" << message << "}:" << result << std::endl;
    return result;
}

std::optional<std::string> Receive(zmq::socket_t& sender) {
    zmq::message_t request;
    if (sender.recv(request, zmq::recv_flags::none).has_value()) {
        std::cout << "    recv{" << request.to_string() << "}" << std::endl;
        return {request.to_string()};
    }
    return std::nullopt;
}