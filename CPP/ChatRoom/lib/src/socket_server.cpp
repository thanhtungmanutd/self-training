#include "socket_server.h"

void Server::Binding() {
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(this->port);

    if (bind(this->sockfd, (struct sockaddr *)&this->addr, sizeof(this->addr)) < 0) {
        perror("bind failed");
    }
    std::cout << "Chat Server: bind server socket done\r\n";
}

void Server::Listen() {
    if (listen(this->sockfd, 10) < 0) {
        perror("listen failed");
    }
    std::cout << "Chat Server: start listening to clients\r\n";
}

int Server::MakeSocketNonBlocking(int& sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

void Server::AddToEpoll(int& fd) {
    this->ev.events = EPOLLIN | EPOLLET;
    this->ev.data.fd = fd;
    epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    this->con_list.insert({fd, REQUEST});
}

void Server::DelFromEpoll(int& fd) {
    epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    this->con_list.erase(fd);
}

void Server::NotifyToOnelineUser(int __sockfd, std::string msg) {
    SQLite::Statement query(this->dtb.db, "SELECT sockfd FROM usrs WHERE status = 1;");

    while (query.executeStep()) {
        int sockfd = sockfd = query.getColumn(0).getInt(); ;
        if (sockfd != __sockfd) {
            std::cout << sockfd << std::endl;
            // send(sockfd, msg.c_str(), msg.size(), 0);
        }
    }
}

void Server::Init(const char *address) {
    this->sockfd = socket(AF_INET, this->type, 0);
    if (this->sockfd < 0) {
        perror("socket failed");
    }
    std::cout << "Chat Server: create socket done\r\n";

    int opt = 1;
    if (setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("setsockopt reuse address failed");
        close(this->sockfd);
        exit(EXIT_FAILURE);
    }
    
    this->addr.sin_addr.s_addr = inet_addr(address);
    Binding();
    if (this->type == SOCK_STREAM) {
        Listen();
    }

    if (MakeSocketNonBlocking(this->sockfd) == -1) {
        close(this->sockfd);
        exit(EXIT_FAILURE);
    }

    this->epoll_fd = epoll_create1(0);
    if (this->epoll_fd == -1) {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = this->sockfd;
    if (epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, this->sockfd, &ev) == -1) {
        perror("epoll_ctl failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Chat Server: Server Port Information: " << inet_ntoa(this->addr.sin_addr) << ":" << this->port << "\r\n";
}

int Server::Accept(struct sockaddr_in& client_addr, socklen_t& len) {
    int confd = accept(this->sockfd, (struct sockaddr *)&client_addr, &len);

    if (confd < 0) {
        std::cout << "Chat Server: fail to connect to client\r\n";
        return 0;
    } 
    return confd;
}

Response Server::CheckUserInfo(RequestMsg& msg, int& confd) {
    std::string pass; int status,fd;
    Response res;

    switch (msg.option) {
        case '1':
            if (!this->dtb.QueryUserInfo(std::string(msg.name), pass, status, fd)) {
                res = ERROR_USERNAME_NOT_FOUND;
            } else {
                if (pass.compare(std::string(msg.pass)) == 0) {
                    if (status == 0) {
                        this->dtb.UpdateUserStatus(std::string(msg.name), 1, std::move(sockfd));
                        res = SUCCESS;
                    } else if (status == 1) {
                        res = ERROR_USER_LOGGED_IN;
                    }
                } else {
                    res = ERROR_PASSWORD_NOT_MATCHING;
                }
            }
            break;
    
        case '2':
            if (!this->dtb.QueryUserInfo(std::string(msg.name), pass, status, fd)) {
                if (this->dtb.AddNewUserToDatabase(std::string(msg.name), std::string(msg.pass), 1, confd)) 
                    res = SUCCESS;
                else 
                    res = ERROR_UNKOWN;
            } else {
                res = ERROR_DUPLICATE_USERNAME; 
            }
            break;

        default:
            break;
    }
    return res;
}

void Server::HandleSingleCon(int fd) {
    State state = this->con_list[fd];
    RequestMsg reqmsg;
    switch(state) {
        case REQUEST: {
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            std::string msg = " ";

            int bytes = read(fd, &reqmsg, sizeof(reqmsg));
            if (reqmsg.option == '2') 
                msg.assign("Chat Server: Incoming registration request at ");
            else if (reqmsg.option == '1')
                msg.assign("Chat Server: Incoming Login request at ");
            
            getpeername(fd, (struct sockaddr*)&addr, &addr_len);
            std::cout << msg << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << "\r\n";
            Response res = CheckUserInfo(reqmsg, fd);
            send(fd, &res, sizeof(res), 0);
            if (res == SUCCESS) {
                std::cout << "Chat Server: " <<  reqmsg.name << " joined the Chat Room at "  << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << "\r\n";
                this->con_list[fd] = AUTHENTICATED;
            }
            break;
        }

        case AUTHENTICATED: {
            int option;
            std::string msg = " ";
            int bytes = read(fd, &option, sizeof(option));
            
            if ((bytes <= 0) || (option == 3)) {
                std::cout << "Chat Server: " << reqmsg.name << " left the Chat Room\r\n";
                msg = std::format("INFO: {} is offline\r\n", std::string(reqmsg.name)); 
                NotifyToOnelineUser(fd, msg);
                this->dtb.UpdateUserStatus(std::string(reqmsg.name), 0, -1);
                DelFromEpoll(fd);
            } else {
                if (option == 1) 
                    std::cout << "Chat Server: " << reqmsg.name << " selected Single user chat\r\n";
                if (option == 2)
                    std::cout << "Chat Server: " << reqmsg.name << " selected Multi user chat\r\n";
                Response res = SUCCESS;
                send(fd, &res, sizeof(res), 0);
                msg = std::format("INFO: {} is online\r\n", std::string(reqmsg.name)); 
                // std::cout << msg <<std::endl;
                // NotifyToOnelineUser(fd, msg);
                this->con_list[fd] = ONLINE;
            }
            break;
        }

        case ONLINE:
            break;

        default:
            break;
    }
}

void Server::HandleConnections() {
    this->alive = true;
    struct epoll_event ev, events[MAX_EVENTS];

    while (this->alive) {
        this->nsockfds = epoll_wait(this->epoll_fd, this->events, MAX_EVENTS, -1);
        if (this->nsockfds == -1) {
            perror("epoll_wait failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < this->nsockfds; ++i) {
            if (this->events[i].data.fd == this->sockfd) {
                // new client
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);
                int confd = Accept(client_addr, len);
                if (confd == -1) {
                    perror("accept failed");
                    continue;
                }
                // printf("Accepted new client: fd=%d\n", confd);
                MakeSocketNonBlocking(confd);
                AddToEpoll(confd);
            } else {
                HandleSingleCon(this->events[i].data.fd);
            }
        }
    }
}