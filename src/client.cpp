#include "Client.hpp"

int main() {
    Client client("auth", "chat", "group");
    client.run();
}