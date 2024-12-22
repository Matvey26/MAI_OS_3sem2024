#pragma once

#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

#include "json.hpp" // Библиотека для работы с JSON (https://github.com/nlohmann/json)
#include "Connection.hpp"

using json = nlohmann::json;

auto cur_time() {
    return std::chrono::high_resolution_clock::now();
}

class Client {
public:
    Client(const std::string& auth_name, const std::string& chat_name, const std::string& group_name)
        : auth_connection(std::make_unique<Connection>(auth_name, Connection::CLIENT)),
          chat_connection(std::make_unique<Connection>(chat_name, Connection::SEND_ONLY)),
          group_connection(std::make_unique<Connection>(group_name, Connection::SEND_ONLY)),
          running(true) {}

    void run() {
        std::thread receiver_thread(&Client::receive_messages, this);
        std::string command;
        while (running) {
            std::cout << "> ";
            std::getline(std::cin, command);
            if (command == "login") {
                login();
            } else if (command == "send_message") {
                send_message();
            } else if (command == "create_group") {
                create_group();
            } else if (command == "join_group") {
                join_group();
            } else if (command == "send_group_message") {
                send_group_message();
            }
            //  else if (command == "show_chat_list") {
            //     show_chat_list();
            // } else if (command == "show_group_list") {
            //     show_group_list();
            // } else if (command == "open_chat") {
            //     open_chat();
            // } else if (command == "open_group") {
            //     open_group();
            // }
            
            else if (command == "quit") {
                running = false;
            } else {
                std::cout << "Unknown command. Available commands: login, send_message, create_group, join_group, send_group_message, quit" << std::endl;
            }
        }
        receiver_thread.join();
    }

private:
    void login() {
        std::string username, password;
        std::cout << "Username: ";
        std::getline(std::cin, username);
        std::cout << "Password: ";
        std::getline(std::cin, password);

        json request = {
            {"action", "login"},
            {"username", username},
            {"password", password}
        };

        auth_connection->send(request.dump());

        while (auth_connection->empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::string response = auth_connection->receive();
        json parsed_response = json::parse(response);

        if (parsed_response["status"] == "success") {
            token = parsed_response["token"];
            response_connection = std::make_unique<Connection>(parsed_response["connection_name"], Connection::RECEIVE_ONLY);
            std::cout << "Login successful. Token received." << std::endl;
        } else {
            std::cout << "Login failed: " << parsed_response["message"] << std::endl;
        }
    }

    void send_message() {
        if (token.empty()) {
            std::cout << "You must log in first!" << std::endl;
            return;
        }

        std::string recipient, message;
        std::cout << "Recipient username: ";
        std::getline(std::cin, recipient);
        std::cout << "Message: ";
        std::getline(std::cin, message);

        json request = {
            {"action", "message"},
            {"token", token},
            {"to_user", recipient},
            {"message", message}
        };
        chat_connection->send(request.dump());
        std::cout << "Message sent to " << recipient << "." << std::endl;
    }

    void create_group() {
        if (token.empty()) {
            std::cout << "You must log in first!" << std::endl;
            return;
        }

        std::string group_name;
        std::cout << "Group name: ";
        std::getline(std::cin, group_name);

        json request = {
            {"action", "create_group"},
            {"token", token},
            {"group_name", group_name}
        };
        group_connection->send(request.dump());
        std::cout << "Group " << group_name << " created." << std::endl;
    }

    void join_group() {
        if (token.empty()) {
            std::cout << "You must log in first!" << std::endl;
            return;
        }

        std::string group_name;
        std::cout << "Group name: ";
        std::getline(std::cin, group_name);

        json request = {
            {"action", "join_group"},
            {"token", token},
            {"group_name", group_name}
        };
        group_connection->send(request.dump());
        std::cout << "Joined group " << group_name << "." << std::endl;
    }

    void send_group_message() {
        if (token.empty()) {
            std::cout << "You must log in first!" << std::endl;
            return;
        }

        std::string group_name, message;
        std::cout << "Group name: ";
        std::getline(std::cin, group_name);
        std::cout << "Message: ";
        std::getline(std::cin, message);

        json request = {
            {"action", "message"},
            {"token", token},
            {"group_name", group_name},
            {"message", message}
        };
        group_connection->send(request.dump());
        std::cout << "Message sent to group " << group_name << "." << std::endl;
    }

    void receive_messages() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            try {
                if (!response_connection or response_connection->empty()) continue;
                std::string raw_message = response_connection->receive();
                if (!raw_message.empty()) {
                    json message = json::parse(raw_message);
                    std::string chat_or_group = (message["type"] == "group") ? message["group_name"] : message["from"];
                    messages[chat_or_group].push_back(message["message"]);
                    std::cout << "\nNew message in " << chat_or_group << ": " << message["message"] << std::endl;
                }
            } catch (...) {
                // Handle any exceptions (e.g., timeout or disconnect)
            }
        }
    }
private:
    std::string token;
    std::map<std::string, std::vector<std::string>> messages; // Хранение сообщений по чатам и группам
    std::atomic<bool> running;

    std::unique_ptr<Connection> auth_connection;
    std::unique_ptr<Connection> chat_connection;
    std::unique_ptr<Connection> group_connection;

    std::unique_ptr<Connection> response_connection;
};
