#pragma once

#include <zmq.hpp>
#include <optional>
#include <iostream>
#include <set>
#include <vector>
#include <string>
#include <future>
#include <stdexcept>

#include "SocketCommunication.hpp"
#include "SusBinHeap.hpp"
#include "Utils.hpp"

class ControlNode {
private:
    // Для работы с топологией
    SusBinHeap topology;

    // Для общения с корневым вычислительным узлом
    zmq::context_t context;
    std::string port;
    zmq::socket_t socket;

    // Для асинхронной обработки
    std::unordered_map<int, std::promise<std::string>> request_promise_map;
    std::vector<std::future<void>> futures;

    bool running;  // Для включения/выключения

private:
    std::string ping(const std::string& id) {
        if (!topology.find(std::stoi(id))) {
            return "Error: Not found";
        }

        int req_id = generate_request_id();
        std::string path = topology.get_path_to(std::stoi(id));
        std::string message = join({std::to_string(req_id), "ping", path, id}, ";");

        std::promise<std::string> promise;
        std::future<std::string> future = promise.get_future();
        request_promise_map[req_id] = std::move(promise);

        Send(socket, message);

        // Ожидание 30 секунд
        if (future.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
            return "Ok: 0";
        }

        return future.get();
    }

    std::string bind(int parent_id, const std::string& side) {
         int req_id = generate_request_id();
        std::string path = topology.get_path_to(parent_id);
        std::string message = join({std::to_string(req_id), "bind", path, side, std::to_string(parent_id)}, ";");

        std::promise<std::string> promise;
        std::future<std::string> future = promise.get_future();
        request_promise_map[req_id] = std::move(promise);

        Send(socket, message);

        return future.get();
    }

    std::string create(const std::string& id) {
        // Проверяем, существует ли узел с таким айди
        int iid = std::stoi(id);
        if (topology.find(iid)) {
            return "Error: Already exists";
        }

        // Добавляем узел в топологию
        topology.add_id(iid);
        std::string parent_port;

        // Выбираем порт для подключения нового узла к родителю
        if (topology.get_parent_id(iid) == -1) {
            parent_port = port;
        } else {
            parent_port = get_free_port();
        }

        // Пингуем родителя, чтобы убедиться, что он существует
        if (topology.get_parent_id(iid) != -1) {
            std::string ping_response = ping(std::to_string(topology.get_parent_id(iid)));
            if (ping_response == "Ok: 0") {
                return "Error: Parent is unavailable";
            }
            if (ping_response == "Error: Not found") {
                return "Error: Parent not found";  // По сути, эта ошибка никогда не будет выведена
            }
        }

        // Отправляем родителю сообщение, чтобы он забиндил свой сокет на нужный порт
        std::cout << "Binding..." << std::endl;
        if (parent_port != port) {
            std::string side = (topology.is_left_child(iid) ? "left" : "right");
            std::string bind_response = bind(topology.get_parent_id(iid), side);

            if (bind_response == "Error") {
                return "Error: Parent cant bind " + side + " socket to port " + parent_port;
            }
        }

        // Создаём процесс вычислительного узла
        pid_t pid = fork();
        if (pid == -1) {
            return "Error: Fork failed";
        }
        if (pid == 0) {
            // Дочерний процесс
            execl("client", id.c_str(), parent_port.c_str(), NULL);

            return "Error: The computing node process has not started";
        } else {
            // Родительский процесс
            return "Ok: " + std::to_string(pid);
        }
    }

    std::string exec(const std::string& id, const std::string& args) {
        return "some exec";
    }

    // Обработка пользовательских команд
    void process_user_command(const std::string& command_message) {
        std::string cmd;
        std::string id;
        std::istringstream iss(command_message);
        iss >> cmd;
        if (cmd == "exit") {
            running = false;
            return;
        }
        iss >> id;
        if (cmd == "create") {
            futures.push_back(std::async(
                std::launch::async,
                [this, id](){
                    std::cout << create(id) << std::endl;
                }
            ));
        }
        if (cmd == "ping") {
            futures.push_back(std::async(
                std::launch::async,
                [this, id](){
                    std::cout << ping(id) << std::endl;
                }
            ));
        }
        if (cmd == "exec") {
            std::string args;
            std::getline(iss, args);
            futures.push_back(std::async(
                std::launch::async,
                [this, id, args](){
                    std::cout << exec(id, args) << std::endl;
                }
            ));
        }
    }

public:
    ControlNode(const std::string& port)
    : topology()
    , context(1)
    , port(port)
    , socket(context, zmq::socket_type::pair)
    , request_promise_map()
    , futures()
    , running(true)
    {
        socket.bind("tcp://*:" + port);
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
            std::string request;
            std::getline(std::cin, request);

            std::cout << "Processing user request: " << request << std::endl;
            process_user_command(request);
        }

        for (auto& future : futures) {
            future.get();
        }
    }
};