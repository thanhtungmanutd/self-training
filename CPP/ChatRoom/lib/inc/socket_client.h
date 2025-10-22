#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <iostream>
#include <thread>
#include <csignal>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t option;
    char name[100];
    char pass[100];
} RequestMsg;
#pragma pack(pop)

enum Response {SUCCESS,
               ERROR_DUPLICATE_USERNAME,
               ERROR_USERNAME_NOT_FOUND,
               ERROR_PASSWORD_NOT_MATCHING,
               ERROR_USER_LOGGED_IN,
               ERROR_UNKOWN};

class Client {
    private:
        enum State {SENREQUEST, SELECTCHATOPT, ONLINE};
        int port = 80;
        int type = SOCK_STREAM;
        int sockfd = -1;
        int connection_timeout = 5;
        struct sockaddr_in addr;
        std::thread sendThread;
        std::thread receiveThread;
        State state;

    private:
        static void SignalHandler(int signum);
        Response SendRequest(RequestMsg& reqmsg);


    public:
        Client(int&& __port, int&& __type) : port(__port), type(__type) {};
        void Init();
        bool Connect(const char *addr);
        void HandleConnection();
};
#endif /* _CLIENT_H_ */