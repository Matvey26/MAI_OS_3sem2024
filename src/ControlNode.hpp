#pragma once

#include <zmq.hpp>
#include <optional>
#include <iostream>
#include <set>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "SocketCommunication.hpp"

class ControlNode {
    int current_free;  // Текущее свободное место для добавления
    std::vector<int> ids;  // Двоичная куча на массиве
    std::set<int> unique_ids;  // Для проверки наличия id в системе
    std::optional<zmq::socket_t> socket;  // Для общения с корневым вычислительным узлом
    std::string port;

    zmq::context_t context;
    std::mutex command_mutex;
    std::condition_variable command_cv;
    std::vector<std::string> command_queue;

    bool running;

private:
    // Обработка пользовательских команд
    void process_user_command(const std::string& command_message) {
        std::string cmd;
        int id;
        std::istringstream iss(command_message);
        iss >> cmd;
        if (cmd == "exit") {
            running = false;
            return;
        }
        iss >> id;
        if (cmd == "create") {
            if (current_free == 0) {
                pid_t pid = fork();
                if (pid == 0) {
                    
                }
            } else {

            }
        }
    }

public:
    ControlNode(const std::string& port)
    : current_free(0)
    , ids(10, -1)
    , unique_ids()
    , port(port)
    , context(1)
    , running(true)
    {
        socket.emplace(context, zmq::socket_type::pair);
        socket->bind("tcp://*:" + port);
    }

    void run() {
        while (running) {
            // Обработка сообщений от узлов
            while (true) {
                std::optional<std::string> message = Receive(socket);
                if (message.has_value()) {
                    std::cout << "Message from node: " << *message << std::endl;
                } else {
                    break;
                }
            }

            // Обработка пользовательских команд
            std::string command;
            std::getline(std::cin, command);

            std::cout << "Processing user command: " << command << std::endl;
            process_user_command(command);
        }
    }
};