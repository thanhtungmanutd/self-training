#include <thread>
#include "socket.h"
#include <vector>
#include <errno.h>

#define DATABASE_PATH           "database.db"

std::vector<std::thread> threadPool;

int main(int argc, char* args[]) {
    if (argc != 2) {
        std::cout << "Server: Invalid number of arguments (must be 2)\r\n";
        return 1;
    }

    auto server = new Server(atoi(args[1]), SOCK_STREAM, DATABASE_PATH);
    server->Init();

    // while (1) {
    //     sockaddr_in clientAddr; 
    //     socklen_t len = sizeof(clientAddr);

    //     int new_sockfd = server.Accept(clientAddr, len);

    //     if (new_sockfd  > 0) {
    //         std::thread client_thread(&Server::HandleConnection,
    //                                   &server,
    //                                   new_sockfd,
    //                                   clientAddr);
    //         threadPool.push_back(std::move(client_thread));
    //     }
    // }
    // for (std::thread& Thread : threadPool) {
    //     Thread.join();
    // }
}