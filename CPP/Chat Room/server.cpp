#include <thread>
#include "tcp_socket.h"
#include <vector>
#include <errno.h>

std::vector<std::thread> threadPool;

int main(int argc, char* args[]) {
    if (argc != 2) {
        cout << "Server: Invalid number of arguments (must be 2)\r\n";
        return 1;
    }

    Server server(atoi(args[1]));
    server.Init("127.0.0.1");

    while (1) {
        sockaddr_in clientAddr; 
        socklen_t len = sizeof(clientAddr);

        int new_sockfd = server.Accept(clientAddr, len);

        if (new_sockfd  > 0) {
            std::thread client_thread(&Server::HandleConnection,
                                      &server,
                                      new_sockfd,
                                      clientAddr);
            threadPool.push_back(std::move(client_thread));
        }
    }
    for (std::thread& Thread : threadPool) {
        Thread.join();
    }
}