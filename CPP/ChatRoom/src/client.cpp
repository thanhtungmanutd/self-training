#include "socket_client.h"
#include "ncurses.h"

int main(int argc, char* args[]) {
    if (argc != 2) {
        std::cout << "Client: Invalid number of arguments (must be 2)\r\n";
        return 1;
    }

    auto client = new Client(atoi(args[1]), SOCK_STREAM);
    client->Init();

    if (client->Connect("127.0.0.1")) {
        client->HandleConnection();
    }
    return 0;
}