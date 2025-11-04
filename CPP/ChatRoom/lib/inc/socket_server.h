#ifndef _SERVER_H_
#define _SERVER_H_

#include <iostream>
#include <string>
#include <unordered_map>
#include <format>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "database.h"

#define DEFAULT_ADDRESS             "0.0.0.0"
#define MAX_EVENTS                  1000

#define DEFAULT_REQMSG              {0, "", ""} 

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

class Server {
    enum State {REQUEST, AUTHENTICATED, ONLINE};
    private:
        bool alive = false;
        int port, type, sockfd, epoll_fd, nsockfds;
        struct epoll_event ev, events[MAX_EVENTS];
        struct sockaddr_in addr;
        MyDatabase dtb;
        std::unordered_map<int, std::pair<std::string, State>> con_list;

    private:
        void Binding();
        void Listen();
        int MakeSocketNonBlocking(int& sockfd);
        void AddSocketToEpoll(int& fd);
        void DelFromEpoll(int& fd);
        void NotifyToOnelineUser(int& __sockfd, const std::string& msg);
        Response CheckUserInfo(RequestMsg& msg, int& fd);

    public:
        Server(int&& __port, int&& __type, std::string&& __path) : port(__port), type(__type), dtb(MyDatabase(std::move(__path))) {}
        void Init(const char *address = DEFAULT_ADDRESS);
        int Accept(sockaddr_in& client_addr, socklen_t& len);
        void HandleConnections();
        void HandleSingleSocketCon(int fd);
};

#endif /* _SERVER_H_ */