#pragma once

#include <zmq.hpp>
#include <optional>
#include <iostream>
#include <string>
#include <future>
#include <vector>

#include "SocketCommunication.hpp"
#include "Utils.hpp"

class TreeNode {
private:
    int id;
    std::optional<zmq::socket_t> parent_socket;
    std::optional<zmq::socket_t> left_socket;
    std::optional<zmq::socket_t> right_socket;

    zmq::context_t context;

    // std::vector<std::future<void>>     

    bool running;

private:
    void proccess_message(const std::string& message) {

    }

public:
    TreeNode(int node_id) : id(node_id), context(1) {}

    void setup_parent_socket(const std::string& port) {
        if (!parent_socket.has_value()) {
            parent_socket.emplace(context, zmq::socket_type::pair);
            parent_socket->connect("tcp://localhost:" + port);
            std::cout << "Parent socket connected to tcp://localhost:" << port << std::endl;
        }
    }

    void setup_left_socket(const std::string& port) {
        if (!left_socket.has_value()) {
            left_socket.emplace(context, zmq::socket_type::pair);
            left_socket->bind("tcp://*:" + port);
            std::cout << "Left socket bound to tcp://*:" << port << std::endl;
        }
    }

    void setup_right_socket(const std::string& port) {
        if (!right_socket.has_value()) {
            right_socket.emplace(context, zmq::socket_type::pair);
            right_socket->bind("tcp://*:" + port);
            std::cout << "Right socket bound to tcp://*:" << port << std::endl;
        }
    }

    void run() {
        while (running) {
            // Обработка сообщений от узлов
            while (parent_socket.has_value()) {
                std::optional<std::string> message = Receive(parent_socket.value());
                if (message.has_value()) {
                    proccess_message(message.value()); 
                } else {
                    break;
                }
            }

            // Пересылаем сообщения от детей к родителю
            while (left_socket.has_value()) {
                std::optional<std::string> message = Receive(left_socket.value());
                if (message.has_value()) {
                    Send(parent_socket.value(), message.value());
                } else {
                    break;
                }
            }

            // Пересылаем сообщения от детей к родителю
            while (right_socket.has_value()) {
                std::optional<std::string> message = Receive(right_socket.value());
                if (message.has_value()) {
                    Send(parent_socket.value(), message.value());
                } else {
                    break;
                }
            }
        }

        // for (auto& future : futures) {
        //     future.get();
        // }
    }
};

// int main() {
//     TreeNode parent(1);
//     TreeNode leftChild(2);
//     TreeNode rightChild(3);

//     // Пример настройки сокетов для узлов
//     parent.setup_parent_socket("5555");
//     parent.setup_left_socket("5556");
//     parent.setup_right_socket("5557");

//     // Пример подключения дочерних узлов к родительскому узлу
//     leftChild.setup_parent_socket("5556");
//     rightChild.setup_parent_socket("5557");

//     return 0;
// }
