#ifndef _TCP_SOCKET_H_
#define _TCP_SOCKET_H_

#include <csignal>
#include <iostream>
#include <string>
#include <cstdio>
#include <sqlite3.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <thread>

using namespace std;

enum class Response {SUCCESS,
                     ERROR_DUPLICATE_USERNAME,
                     ERROR_USERNAME_NOT_FOUND,
                     ERROR_PASSWORD_NOT_MATCHING,
                     ERROR_USER_LOGGED_IN,
                     ERROR_UNKOWN};

typedef struct {
    char username[100];
    char password[100];
    int option;
} request_message_t;

class Socket {
    public:
        int sockfd;
        int port;
        sockaddr_in server_addr;
        Socket(int _port) : port(_port) {};
};

class Database {
    private:
        sqlite3 *database;
    
    public:
        Database();
        sqlite3* SQLite3() {return this->database;}
        bool QuerryUserInfo(string&& name,
                            string& pass,
                            int& status,
                            int& sockfd);
        bool UpdateUserStatus(string&& name, int&& status, int&& sockfd);
        bool AddNewUserToDatabase(string&& name,
                                  string&& pass,
                                  int&& status,
                                  int& sockfd);
};

class Server : private Socket {
    enum class State {recvClientReq, recvClientChatOpt, Online};
    private:
        Database database;
        State state;
        void Binding();
        void Listen();

    public:
        Server(int _port) : Socket(_port) {};
        void Init(const char *address);
        int Accept(sockaddr_in& client_addr, socklen_t& len);
        Response CheckUserInfo(int& sockfd, request_message_t& msg);
        void NotifyToOnelineUser(int new_sockfd, string msg);
        void HandleConnection(int new_sockfd, sockaddr_in client_addr);
};

class Client : private Socket {
    enum class State {sendReq, SelectChatOpt, Online};
    private:
        thread sendThread;
        thread receiveThread;
        State state;
        Response SendRequest(request_message_t& req);
    public:
        Client(int _port) : Socket(_port) {};
        ~Client();
        void Init();
        bool Connect(const char *addr);
        void HandleConnection();
        void ReceiveFromServer();
        void SendToServer();
};

#endif /* _TCP_SOCKET_H_ */