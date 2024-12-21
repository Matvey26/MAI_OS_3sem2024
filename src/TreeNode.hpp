#pragma once

#include <zmq.hpp>
#include <optional>
#include <iostream>
#include <string>
#include <future>
#include <vector>
#include <sstream>

#include "SocketCommunication.hpp"
#include "Utils.hpp"

class TreeNode {
private:
    int id;
    std::optional<zmq::socket_t> parent_socket;
    std::optional<zmq::socket_t> left_socket;
    std::optional<zmq::socket_t> right_socket;

    zmq::context_t context;

    zmq::pollitem_t ps;
    zmq::pollitem_t ls;
    zmq::pollitem_t rs;

    bool running;

private:
    void process_message(const std::string& message) {
        if (message == "recursive_destroy") {
            running = false;
            if (left_socket.has_value()) {
                Send(left_socket.value(), "recursive_destroy");
                left_socket = std::nullopt;
            }
            if (right_socket.has_value()) {
                Send(right_socket.value(), "recursive_destroy");
                right_socket = std::nullopt;
            }

            return;
        }
        std::vector<std::string> tokens = split(message, ';');
        std::string request_id = tokens[0];
        std::string cmd = tokens[1];
        std::string path = tokens[2];
        std::string id = tokens[3];

        if (path.size() != 0) {
            tokens[2] = path.substr(1, path.size() - 1);

            if (path[0] == 'l' and left_socket.has_value()) {
                Send(left_socket.value(), join(tokens, ";"));
            } else if (path[0] == 'r' and right_socket.has_value()) {
                Send(left_socket.value(), join(tokens, ";"));
            } else {
                Send(parent_socket.value(), join({request_id, "Error: Not found"}, ";"));
            }
        }

        if (cmd == "ping") {
            Send(parent_socket.value(), join({request_id, "Ok: 1"}, ";"));
        } else if (cmd == "bind") {
            std::string side = tokens[4];
            std::string port = tokens[5];
            if (side == "left") {
                setup_left_socket(port);
            } else if (side == "right") {
                setup_right_socket(port);
            }
            Send(parent_socket.value(), join({request_id, "Ok: Binded"}, ";"));
        } else if (cmd == "exec") {
            std::stringstream iss(tokens[4]);
            int n;
            iss >> n;
            int sum = 0;
            for (int i = 0; i < n; ++i) {
                int cur;
                iss >> cur;
                sum += cur;
            }
            std::string message = "Ok:" + std::to_string(this->id) + ": " + std::to_string(sum);
            Send(parent_socket.value(), join({request_id, message}, ";"));
        } else {
            Send(parent_socket.value(), join({request_id, "Error: Unknown command"}, ";"));
        }
    }

public:
    TreeNode(int node_id) : id(node_id), context(1), running(true) {}

    void setup_parent_socket(const std::string& port) {
        if (!parent_socket.has_value()) {
            parent_socket.emplace(context, ZMQ_PAIR);
            parent_socket->bind("tcp://*:" + port);
            ps = zmq::pollitem_t{parent_socket.value(), 0, ZMQ_POLLIN, 0};
            std::cout << "Parent socket connected to tcp://*:" << port << std::endl;
        }
    }

    void setup_left_socket(const std::string& port) {
        if (!left_socket.has_value()) {
            left_socket.emplace(context, ZMQ_PAIR);
            left_socket->connect("tcp://localhost:" + port);
            ls = zmq::pollitem_t{left_socket.value(), 0, ZMQ_POLLIN, 0};
            std::cout << "Left socket bound to tcp://localhost:" << port << std::endl;
        }
    }

    void setup_right_socket(const std::string& port) {
        if (!right_socket.has_value()) {
            right_socket.emplace(context, ZMQ_PAIR);
            right_socket->connect("tcp://localhost:" + port);
            rs = zmq::pollitem_t{right_socket.value(), 0, ZMQ_POLLIN, 0};
            std::cout << "Right socket bound to tcp://localhost:" << port << std::endl;
        }
    }

    void run() {
        while (running) {
            // Обработка сообщений от родителя
            if (parent_socket.has_value()) {
                zmq::pollitem_t items[] = {ps};
                zmq::poll(items, 1, std::chrono::milliseconds(500));

                if (items[0].revents & ZMQ_POLLIN) {
                    std::optional<std::string> message = Receive(parent_socket.value());
                    if (message.has_value()) {
                        std::cout << "[COMP_NODE] Processing " << message.value() << std::endl;
                        process_message(message.value());
                    }
                }
            }

            // Пересылаем сообщения от левого дочернего узла к родителю
            if (left_socket.has_value()) {
                zmq::pollitem_t items[] = {ls};
                zmq::poll(items, 1, std::chrono::milliseconds(500));

                if (items[0].revents & ZMQ_POLLIN) {
                    std::optional<std::string> message = Receive(left_socket.value());
                    if (message.has_value()) {
                        std::cout << "[COMP_NODE] Resend " << message.value() << std::endl;
                        Send(parent_socket.value(), message.value());
                    }
                }
            }

            // Пересылаем сообщения от правого дочернего узла к родителю
            if (right_socket.has_value()) {
                zmq::pollitem_t items[] = {rs};
                zmq::poll(items, 1, std::chrono::milliseconds(500));

                if (items[0].revents & ZMQ_POLLIN) {
                    std::optional<std::string> message = Receive(right_socket.value());
                    if (message.has_value()) {
                        std::cout << "[COMP_NODE] Resend " << message.value() << std::endl;
                        Send(parent_socket.value(), message.value());
                    }
                }
            }
        }
    }


    ~TreeNode() {
        std::cout << "Destroying Node " << std::to_string(id) << std::endl;
        if (left_socket.has_value())
            Send(left_socket.value(), "recursive_destroy");
        if (right_socket.has_value())
            Send(right_socket.value(), "recursive_destroy");
    }
};
