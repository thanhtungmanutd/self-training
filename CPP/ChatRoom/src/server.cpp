#include "socket_server.h"

#define DATABASE_PATH           "../database.db"

int main(int argc, char* args[]) {
    if (argc != 2) {
        std::cout << "Server: Invalid number of arguments (must be 2)\r\n";
        return 1;
    }

    auto server = new Server(atoi(args[1]), SOCK_STREAM, DATABASE_PATH);
    server->Init("127.0.0.1");

    server->HandleConnections();

    return 0;
}