#include <iostream>
#include <future>
#include <thread>
#include <unordered_map>
#include <zmq.hpp>
#include <chrono>
#include <string>
#include <atomic>

// Глобальный контекст ZMQ
zmq::context_t context(1);

// Сокет для отправки запросов
zmq::socket_t requester(context, ZMQ_REQ);

// Сокет для получения ответов
zmq::socket_t receiver(context, ZMQ_REP);

// Хранилище для промисов
std::unordered_map<int, std::promise<std::string>> promiseMap;

// Флаг для завершения работы потоков
std::atomic<bool> running(true);

// Функция для отправки запроса и получения ответа
std::future<std::string> sendRequest(const std::string& request) {
    static int requestIdCounter = 0;
    int requestId = requestIdCounter++;

    // Создать промис и получить фьючер
    std::promise<std::string> promise;
    std::future<std::string> future = promise.get_future();

    // Сохранить промис в хранилище
    promiseMap[requestId] = std::move(promise);

    // Отправить запрос с идентификатором
    std::this_thread::sleep_for(std::chrono::seconds(10)); // Имитация долгой работы
    zmq::message_t requestIdMsg(sizeof(requestId));
    std::memcpy(requestIdMsg.data(), &requestId, sizeof(requestId));
    requester.send(requestIdMsg, zmq::send_flags::sndmore);

    zmq::message_t requestMsg(request.size());
    std::memcpy(requestMsg.data(), request.data(), request.size());
    requester.send(requestMsg, zmq::send_flags::none);

    return future;
}

// Функция для обработки входящих сообщений
void handleIncomingMessages() {
    while (running) {
        zmq::pollitem_t items[] = {
            {receiver, 0, ZMQ_POLLIN, 0}
        };
        zmq::poll(items, 1, std::chrono::milliseconds(100));

        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t requestIdMsg;
            auto result = receiver.recv(requestIdMsg, zmq::recv_flags::none);
            if (!result) {
                std::cerr << "Failed to receive request ID message" << std::endl;
                continue;
            }
            int requestId = *static_cast<int*>(requestIdMsg.data());

            zmq::message_t responseMsg;
            result = receiver.recv(responseMsg, zmq::recv_flags::none);
            if (!result) {
                std::cerr << "Failed to receive response message" << std::endl;
                continue;
            }
            std::string response(static_cast<char*>(responseMsg.data()), responseMsg.size());

            // Найти промис по идентификатору запроса и установить значение
            if (promiseMap.find(requestId) != promiseMap.end()) {
                promiseMap[requestId].set_value(response);
                promiseMap.erase(requestId);
            }
        }
    }
}

// Функция для обработки команд пользователя
void handleUserCommands() {
    std::string command;
    while (running) {
        std::cout << "Enter command: ";
        std::getline(std::cin, command);

        if (command == "exit") {
            running = false;
            break;
        } else if (command == "send") {
            std::string request;
            std::cout << "Enter request: ";
            std::getline(std::cin, request);

            std::future<std::string> responseFuture = sendRequest(request);

            std::string response = responseFuture.get();
            std::cout << "Received response: " << response << std::endl;
        } else {
            std::cout << "Unknown command" << std::endl;
        }
    }
}

int main() {
    // Подключение сокетов
    receiver.bind("tcp://*:5555");
    requester.connect("tcp://localhost:5555");

    // Запуск потока для обработки входящих сообщений
    std::thread messageHandler(handleIncomingMessages);

    // Обработка команд пользователя
    handleUserCommands();

    // Завершение работы
    messageHandler.join();
    return 0;
}
