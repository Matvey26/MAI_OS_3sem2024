#pragma once

#include <iostream>
#include <thread>
#include <unordered_map>
#include <set>

#include "Connection.hpp"
#include "json.hpp" // Библиотека для работы с JSON

using json = nlohmann::json;

class Server {
public:
    Server(const std::string& auth_name, const std::string& chat_name, const std::string& group_name)
        : auth_connection(std::make_unique<Connection>(auth_name, Connection::SERVER)),
          chat_connection(std::make_unique<Connection>(chat_name, Connection::RECEIVE_ONLY)),
          group_connection(std::make_unique<Connection>(group_name, Connection::RECEIVE_ONLY)) {}

    void add_user_connection(const std::string& username) {
        if (user_connections.find(username) == user_connections.end()) {
            user_connections[username] = std::make_unique<Connection>("user_" + username, Connection::SEND_ONLY);
        } else {
            throw std::runtime_error("User connection already exists for: " + username);
        }
    }

    void run() {
        while (true) {
            // Проверяем сообщения из очереди auth
            if (!auth_connection->empty()) {
                std::string auth_request = auth_connection->receive();
                std::cout << "Request{" << auth_request << "}" << std::endl;
                process_auth_request(auth_request);
            }

            // Проверяем сообщения из очереди chat
            if (!chat_connection->empty()) {
                std::string chat_request = chat_connection->receive();
                std::cout << "Request{" << chat_request << "}" << std::endl;
                process_chat_request(chat_request);
            }

            // Проверяем сообщения из очереди group
            if (!group_connection->empty()) {
                std::string group_request = group_connection->receive();
                std::cout << "Request{" << group_request << "}" << std::endl;
                process_group_request(group_request);
            }

            // Небольшая пауза для снижения нагрузки на процессор
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

private:
    std::unique_ptr<Connection> auth_connection;
    std::unique_ptr<Connection> chat_connection;
    std::unique_ptr<Connection> group_connection;

    std::unordered_map<std::string, std::unique_ptr<Connection>> user_connections; // Личные соединения пользователей
    std::unordered_map<std::string, std::set<std::string>> groups; // Карта групп: group_name -> {users}
    std::unordered_map<std::string, std::string> user_tokens; // Карта токенов: username -> token
    std::unordered_map<std::string, std::string> token_user_map; // Обратная карта: token -> username

    std::string generate_token() {
        static size_t counter = 0;
        return "token_" + std::to_string(++counter);
    }

    void process_auth_request(const std::string& request) {
        try {
            json req = json::parse(request);
            if (req["action"] == "login") {
                std::string username = req["username"];
                std::string password = req["password"]; // В реальной жизни пароль нужно проверять!
                
                std::string token = generate_token();
                user_tokens[username] = token;
                token_user_map[token] = username;

                json response = {
                    {"status", "success"},
                    {"token", token},
                    {"connection_name", "user_" + username}
                };

                // Создаем персональное соединение для пользователя
                add_user_connection(username);

                auth_connection->send(response.dump());
                std::cout << "User " << username << " logged in." << std::endl;
            } else {
                throw std::runtime_error("Invalid auth action.");
            }
        } catch (const std::exception& e) {
            json error_response = {
                {"status", "error"},
                {"message", e.what()}
            };
            auth_connection->send(error_response.dump());
        }
    }

    void process_chat_request(const std::string& request) {
        try {
            json req = json::parse(request);
            std::string token = req["token"];
            std::string sender = token_user_map[token];
            std::string recipient = req["to_user"];
            std::string message = req["message"];

            if (user_connections.find(recipient) == user_connections.end()) {
                throw std::runtime_error("Recipient user not found.");
            }

            json chat_message = {
                {"from", sender},
                {"type", "chat"},
                {"message", message}
            };

            user_connections[recipient]->send(chat_message.dump());
            std::cout << "Message from " << sender << " to " << recipient << ": " << message << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Chat error: " << e.what() << std::endl;
        }
    }

    void process_group_request(const std::string& request) {
        try {
            json req = json::parse(request);
            std::string token = req["token"];
            std::string sender = token_user_map[token];
            std::string action = req["action"];

            if (action == "create_group") {
                std::string group_name = req["group_name"];
                if (groups.find(group_name) != groups.end()) {
                    throw std::runtime_error("Group already exists.");
                }
                groups[group_name] = {sender};
                std::cout << "Group " << group_name << " created by " << sender << "." << std::endl;
            } else if (action == "join_group") {
                std::string group_name = req["group_name"];
                if (groups.find(group_name) == groups.end()) {
                    throw std::runtime_error("Group not found.");
                }
                groups[group_name].insert(sender);
                std::cout << "User " << sender << " joined group " << group_name << "." << std::endl;
            } else if (action == "message") {
                std::string group_name = req["group_name"];
                std::string message = req["message"];
                if (groups.find(group_name) == groups.end()) {
                    throw std::runtime_error("Group not found.");
                }

                json group_message = {
                    {"from", sender},
                    {"type", "group"},
                    {"group_name", group_name},
                    {"message", message}
                };

                for (const auto& member : groups[group_name]) {
                    if (member != sender && user_connections.find(member) != user_connections.end()) {
                        user_connections[member]->send(group_message.dump());
                    }
                }
                std::cout << "Message in group " << group_name << " from " << sender << ": " << message << std::endl;
            } else {
                throw std::runtime_error("Invalid group action.");
            }
        } catch (const std::exception& e) {
            std::cerr << "Group error: " << e.what() << std::endl;
        }
    }
};

// Пример использования:
// int main() {
//     Server server("auth", "chat", "group");
//     server.run();
//     return 0;
// }
