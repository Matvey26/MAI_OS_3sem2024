#include "Server.hpp"

int main() {
    Server server("auth", "chat", "group");
    server.run();
}