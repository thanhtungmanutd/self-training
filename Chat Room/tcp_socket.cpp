#include "tcp_socket.h"

#define DATABASE_PATH           "../database.db"
#define LOG_ERROR(msg)          cout << msg << "\r"; \
                                exit(EXIT_FAILURE)

using namespace std;

/*----------------------------------------------------------------------------*/
/*                               DATABASE                                     */
/*----------------------------------------------------------------------------*/
Database::Database() {
    if (sqlite3_open(DATABASE_PATH, &database) != SQLITE_OK) 
        cout << "Chat Server: Failed to connect to databse\r\n"; 
}

bool Database::QuerryUserInfo(string&& name,
                              string& pass,
                              int& status,
                              int& sockfd) {
    string cmd = "SELECT * FROM usrs WHERE username = '" + name + "';";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(database, cmd.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        cout << "Chat Server: Failed to prepare statement to querry data\r\n";
        return false;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        pass = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))); 
        status = sqlite3_column_int(stmt, 2);
        sockfd = sqlite3_column_int(stmt, 3);
        sqlite3_finalize(stmt);
        return true;
    }
    sqlite3_finalize(stmt);
    return false;
}

bool Database::UpdateUserStatus(string&& name, int&& status, int&& sockfd) {
    string cmd = "UPDATE usrs SET status = ?, sockfd = ? WHERE username = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(database, cmd.c_str(), -1, &stmt, 0)) {
        cout << "Chat Server: Failed to prepare statement to update data\r\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, status);
    sqlite3_bind_int(stmt, 2, sockfd);
    sqlite3_bind_text(stmt, 3, name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return true;
    }
    sqlite3_finalize(stmt);
    return false;
}

bool Database::AddNewUserToDatabase(string&& name,
                                    string&& pass,
                                    int&& status,
                                    int& sockfd) {
    char cmd[1000] = {0};

    sprintf(cmd, "INSERT INTO usrs (username, password, status, sockfd) VALUES ('%s', '%s', %d, %d);",
            name.c_str(), pass.c_str(), status, sockfd);
    if (sqlite3_exec(database, cmd, 0, 0, NULL) == SQLITE_OK) {
        return true;
    }
    return false;
}

/*----------------------------------------------------------------------------*/
/*                               SERVER                                       */
/*----------------------------------------------------------------------------*/
void Server::Binding() {
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Server: failed to bind socket\r\n");
    }
    cout << "Server: bind server socket done\r\n";
}

void Server::Listen() {
    if (listen(sockfd, 10) < 0) {
        LOG_ERROR("Server: failed to listen to clients\r\n");
    }
    cout << "Server: start listening to clients\r\n";
}

void Server::Init(const char *address) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOG_ERROR("Server: failed to create socket\r\n");
    }
    cout << "Server: create socket done\r\n";

    server_addr.sin_addr.s_addr = inet_addr(address);
    Binding();
    Listen();

    cout << "Chat Server: Server Port Information: "
         << inet_ntoa(server_addr.sin_addr) << ":" << port << "\r\n";

    database = Database();
}

int Server::Accept(sockaddr_in& client_addr, socklen_t& len) {
    int new_sockfd = accept(sockfd, (sockaddr *)&client_addr, &len);

    if (new_sockfd < 0) {
        cout << "Server: fail to connect to client\r\n";
        return 0;
    } 
    return new_sockfd;
}

Response Server::CheckUserInfo(int& sockfd, request_message_t& msg) {
    string pass; int status,fd;
    Response res;

    switch (msg.option) {
        case 1:
            if (!database.QuerryUserInfo(string(msg.username), pass, status, fd)) {
                res = Response::ERROR_USERNAME_NOT_FOUND;
            } else {
                if (pass.compare(string(msg.password)) == 0) {
                    if (status == 0) {
                        database.UpdateUserStatus(string(msg.username), 1, move(sockfd));
                        res = Response::SUCCESS;
                    } else if (status == 1) {
                        res = Response::ERROR_USER_LOGGED_IN;
                    }
                } else {
                    res = Response::ERROR_PASSWORD_NOT_MATCHING;
                }
            }
            break;
    
        case 2:
            if (!database.QuerryUserInfo(string(msg.username), pass, status, fd)) {
                if (database.AddNewUserToDatabase(string(msg.username), string(msg.password), 1, sockfd)) 
                    res = Response::SUCCESS;
                else 
                    res = Response::ERROR_UNKOWN;
            } else {
                res = Response::ERROR_DUPLICATE_USERNAME; 
            }
            break;

        default:
            break;
    }
    return res;
}

void Server::NotifyToOnelineUser(int new_sockfd, string msg) {
    string cmd = "SELECT sockfd FROM usrs WHERE status = 1;";
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(database.SQLite3(), cmd.c_str(), -1, &stmt, 0);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int sockfd = sqlite3_column_int(stmt, 0);
        if (sockfd != new_sockfd) {
            send(sockfd, msg.c_str(), msg.size(), 0);
        }
    }
}

void Server::HandleConnection(int new_sockfd, sockaddr_in client_addr) {
    bool connection = true;
    int bytes;
    request_message_t req;
    Response res;
    state = State::recvClientReq;
    string msg = " ";

    while (connection) {
        switch (state) {
            case State::recvClientReq:
                if (read(new_sockfd, &req, sizeof(req)) <= 0) {
                    connection = false;
                    close(new_sockfd);
                } else {
                    if (req.option == 2) 
                        msg.assign("Chat Server: Incoming registration request at ");
                    if (req.option == 1)
                        msg.assign("Chat Server: Incoming Login request at ");
                    cout << msg << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "\r\n";
                    res = CheckUserInfo(new_sockfd, req);
                    send(new_sockfd, &res, sizeof(res), 0);
                    if (res == Response::SUCCESS) {
                        cout << "Chat Server: " <<  req.username 
                             << " joined the Chat Room at " 
                             << inet_ntoa(client_addr.sin_addr) << ":"
                             << ntohs(client_addr.sin_port) << "\r\n";
                        state = State::recvClientChatOpt;
                    }
                }
                break;

            case State::recvClientChatOpt:
                int option;
                
                bytes = read(new_sockfd, &option, sizeof(option));
                
                if ((bytes <= 0) || (option == 3)) {
                    cout << "Chat Server: " << req.username << " left the Chat Room\r\n";
                    msg = string("INFO: ") + string(req.username) + string(" is offline\r\n"); 
                    NotifyToOnelineUser(new_sockfd, msg);
                    close(new_sockfd);
                    database.UpdateUserStatus(string(req.username), 0, -1);
                    connection = false;
                } else {
                    if (option == 1) 
                        cout << "Chat Server: " << req.username << " selected Single user chat\r\n";
                    if (option == 2)
                        cout << "Chat Server: " << req.username << " selected Multi user chat\r\n";
                    res = Response::SUCCESS;
                    send(new_sockfd, &res, sizeof(res), 0);
                    msg = string("INFO: ") + string(req.username) + string(" is online\r\n"); 
                    NotifyToOnelineUser(new_sockfd, msg);
                    state = State::Online;
                }
                break;

            case State::Online:
                char recv_buff[1000];

                memset(recv_buff, 0, sizeof(recv_buff));
                bytes = read(new_sockfd, recv_buff, sizeof(recv_buff));

                if (bytes <= 0) { 
                    cout << "Chat Server: " << string(req.username) << " left the Chat Room\r\n";
                    msg = string("INFO: ") + string(req.username) + string(" is offline\r\n"); 
                    NotifyToOnelineUser(new_sockfd, msg);
                    close(new_sockfd);
                    database.UpdateUserStatus(string(req.username), 0, -1);
                    connection = false;
                } else if (bytes > 0) {
                    NotifyToOnelineUser(new_sockfd, string(req.username) + ": " + string(recv_buff));
                }
                break;

            default:
                break;
        }
    }
}
/*----------------------------------------------------------------------------*/
/*                               CLIENT                                       */
/*----------------------------------------------------------------------------*/
void Client::Init() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOG_ERROR("Client: failed to create socket\r\n");
    }
    cout << "Client: create socket done\r\n";
}

Client::~Client() {
    if (this->sockfd > 0) {
        close(this->sockfd);
    }
    cout << "\nClient Terminated\r\n";
}

bool Client::Connect(const char *addr) {
    int timeout = 5;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(addr);
    server_addr.sin_port = htons(port);

    while (timeout-- > 0) {
        int ret = connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr));
        if (ret == 0) {
            return 1;
        }
        sleep(1);    
    }
    cout << "Client: connection timeout 5s\r\n";
    return 0;
}

Response Client::SendRequest(request_message_t& req){
    Response res = Response::SUCCESS;

    if (send(sockfd, &req, sizeof(req), 0) < 0) 
        cout << "Client: failed to send request message\r\n";

    if (read(sockfd, &res, sizeof(res)) < 0) 
        cout << "Client: failed to read response message\r\n";
    return res;
}

void Client::HandleConnection() {
    int option;
    string name;
    State state = State::sendReq;
    bool SubThreadCreated = false;
    
    while (1) {
        switch (state) {
            case State::sendReq:
                Response res;
                request_message_t req;

                cout << "INFO: Connected to the Server\r\n\n";
                cout << "1. Login\r\n2. Register\r\n3. Exit\r\n\n";
                cout << "Enter your Option: ";
                cin >> option;

                if (option == 3) 
                    exit(EXIT_SUCCESS);

                cout << "\r\nEnter User Name: ";
                cin >> req.username;
                cout << "\rEnter Password: ";
                cin >> req.password;
                req.option = option;

                res = SendRequest(req);

                switch (res) {
                    case Response::SUCCESS:
                        if (req.option == 1)
                            cout << "\nINFO: User Login Successful\r\n\n";
                        if (req.option == 2)
                            cout << "\nINFO: User Registration Successful\r\n\n";
                        state = State::SelectChatOpt;
                        break;

                    case Response::ERROR_DUPLICATE_USERNAME:
                        cout << "\nINFO: User Registration Failed (Duplicate User Name)\r\n\n";
                        break;

                    case Response::ERROR_USERNAME_NOT_FOUND: 
                        cout << "\nINFO: User Log In Failed (User Name Not Found)\r\n\n";
                        break;

                    case Response::ERROR_PASSWORD_NOT_MATCHING:
                        cout << "\nINFO: User Log In Failed (Password Not Matching)\r\n\n";
                        break;

                    case Response::ERROR_USER_LOGGED_IN:
                        cout << "\nINFO: User Log In Failed (User Already Logged In)\r\n\n";
                        break;
                    
                    case Response::ERROR_UNKOWN:
                        cout << "\nINFO: Unknown Error\r\n\n";

                    default:
                        break;
                }
                break;

            case State::SelectChatOpt:
                cout << "Chat option:\n\r1: Single User Chat\r\n2: Multi User Chat\r\n3: Exit\r\n\n";
                cout << "Select what you would like to proceed with: ";
                cin >> option;

                if (send(sockfd, &option, sizeof(option), 0) < 0) {
                    LOG_ERROR("Client: failed to send chat option message\r\n");
                }
                if (option == 3) 
                    exit(EXIT_SUCCESS);
                if (read(sockfd, &res, sizeof(res)) < 0) {
                    LOG_ERROR("Client: failed to read chat option message\r\n");
                }
                state = State::Online;
                break;

            case State::Online: {
                if (!SubThreadCreated) {
                    sendThread = thread(&Client::SendToServer, this);
                    receiveThread = thread(&Client::ReceiveFromServer, this);
                    SubThreadCreated = true;
                }  
                break;
            }
                
            default:
                break;
        }
    }
    sendThread.join();
    receiveThread.join();
}

void Client::ReceiveFromServer() {
    int bytes;
    char recv_buff[1000];

    while (1) {
        memset(recv_buff, 0, sizeof(recv_buff));
        bytes = read(sockfd, recv_buff, sizeof(recv_buff));

        if (bytes <= 0) {
            close(sockfd);
            exit(EXIT_FAILURE);
            break;
        } else {
            cout << "\r" << recv_buff << '\n';
        }
    }
}

void Client::SendToServer() {
    string input;
    char *input_str;

    cin.ignore();
    while (1) {
        cout << "> ";
        getline(std::cin, input);

        if (input.size() > 0) {
            input_str = (char*) input.c_str();
            if (send(sockfd, input_str, strlen(input_str), 0) <= 0) {
                LOG_ERROR("Client: failed to send message\r\n");
            }
        }
    }
}