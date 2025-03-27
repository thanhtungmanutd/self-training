#include "tcp_socket.h"

using namespace std;

static Client *client;

int main(int argc, char* args[]) {
    if (argc != 2) {
        cout << "Client: Invalid number of arguments (must be 2)\r\n";
        return 1;
    }

    client = new Client(atoi(args[1]));
    client->Init();

    if (client->Connect("127.0.0.1")) {
        client->HandleConnection();
    }
    return 0;
}