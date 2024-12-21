#include <iostream>
#include <future>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>

std::mutex mtx;

// Функция, которая выполняется асинхронно
void asyncFunction(int command) {
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Симуляция длительной работы
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "Result of command " << command << " is ready." << std::endl;
}

int main() {
    std::vector<std::future<void>> futures;
    int command;

    while (true) {
        std::cout << "Enter command (0 to exit): ";
        std::cin >> command;

        if (command == 0) {
            break;
        }

        // Запуск асинхронной функции
        futures.push_back(std::async(std::launch::async, asyncFunction, command));
    }

    // Ожидание завершения всех асинхронных задач
    for (auto& future : futures) {
        future.get();
    }

    return 0;
}
