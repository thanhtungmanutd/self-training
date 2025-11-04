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

void Server::AddSocketToEpoll(int& fd) {
    this->ev.events = EPOLLIN | EPOLLET;
    this->ev.data.fd = fd;
    epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    this->con_list.insert({fd, {"", REQUEST}});
}

void Server::DelFromEpoll(int& fd) {
    epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    this->con_list.erase(fd);
}

void Server::NotifyToOnelineUser(int& __sockfd, const std::string& msg) {
    SQLite::Statement query(this->dtb.db, "SELECT sockfd FROM usrs WHERE status = 1;");

    while (query.executeStep()) {
        int sockfd = sockfd = query.getColumn(0).getInt(); ;
        if (sockfd != __sockfd) {
            send(sockfd, msg.c_str(), msg.size(), 0);
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

    std::cout << "Chat Server: server port information: " << inet_ntoa(this->addr.sin_addr) << ":" << this->port << "\r\n";
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
    int status,fd;
    Response res;
    std::string name = std::string(msg.name);
    std::string pass = std::string(msg.pass);
    std::string stored_pass;

    if (!this->dtb.QueryUserInfo(name, stored_pass, status, fd)) {
        if (msg.option == 1) res = ERROR_USERNAME_NOT_FOUND;
        if (msg.option == 2) {
            if (this->dtb.AddNewUserToDatabase(name, pass, 1, confd)) 
                res = SUCCESS;
            else 
                res = ERROR_UNKOWN;
        }
    } else {
        if (msg.option == 2) res = ERROR_DUPLICATE_USERNAME;
        if (msg.option == 1) {
            if (pass.compare(stored_pass) == 0) {
                if (status == 0) {
                    this->dtb.UpdateUserStatus(name, 1, std::move(confd));
                    res = SUCCESS;
                }
                if (status == 1) res = ERROR_USER_LOGGED_IN;
                
            } else {
                res = ERROR_PASSWORD_NOT_MATCHING;
            }
        }
    }   
    return res;
}

void Server::HandleSingleSocketCon(int fd) {
    State state = this->con_list[fd].second;
    
    switch(state) {
        case REQUEST: {
            RequestMsg reqmsg;
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);

            int bytes = read(fd, &reqmsg, sizeof(reqmsg));
            if (bytes <= 0) {
                DelFromEpoll(fd);
            } else {
                getpeername(fd, (struct sockaddr*)&addr, &addr_len);
                std::cout << std::format("Chat Server: incoming {} request at {}:{}\r\n",
                                         reqmsg.option == '2' ? "registration" : "login",
                                         inet_ntoa(addr.sin_addr),
                                         ntohs(addr.sin_port));
                Response res = CheckUserInfo(reqmsg, fd);
                this->con_list[fd].first = std::string(reqmsg.name);
                send(fd, &res, sizeof(res), 0);
                if (res == SUCCESS) {
                    std::cout << "Chat Server: " <<  reqmsg.name << " joined the Chat Room at "  << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << "\r\n";
                    this->con_list[fd].second = AUTHENTICATED;
                }
            }
            break;
        }

        case AUTHENTICATED: {
            int option;
            int bytes = read(fd, &option, sizeof(option));
            
            std::string name = this->con_list[fd].first;

            if ((bytes <= 0) || (option == 3)) {
                std::cout << "Chat Server: " << name << " left the Chat Room\r\n";
                NotifyToOnelineUser(fd, std::format("INFO: {} is offline\r\n", name));
                this->dtb.UpdateUserStatus(name, 0, -1);
                DelFromEpoll(fd);
            } else {
                std::cout << std::format("Chat Server: {} selected {} user chat\r\n", name, option == 1 ? "single" : "multi");
                Response res = SUCCESS;
                send(fd, &res, sizeof(res), 0);
                NotifyToOnelineUser(fd, std::format("INFO: {} is online\r\n", name));
                this->con_list[fd].second = ONLINE;
            }
            break;
        }

        case ONLINE: {
            char recv_buff[1000];
            std::string name = this->con_list[fd].first;

            memset(recv_buff, 0, sizeof(recv_buff));
            int bytes = read(fd, recv_buff, sizeof(recv_buff));
            if (bytes <= 0) {
                std::cout << "Chat Server: " << name << " left the Chat Room\r\n";
                this->dtb.UpdateUserStatus(name, 0, -1);
                NotifyToOnelineUser(fd, std::format("INFO: {} is offline\r\n", name));
                DelFromEpoll(fd);
            } else {
                NotifyToOnelineUser(fd, std::format("{}: {}\r\n", name, std::string(recv_buff)));
            }
            break;
        }

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
                MakeSocketNonBlocking(confd);
                AddSocketToEpoll(confd);
            } else {
                HandleSingleSocketCon(this->events[i].data.fd);
            }
        }
    }
}