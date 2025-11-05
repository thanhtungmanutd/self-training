#include "socket_client.h"

void Client::SignalHandler(int signum) {
    if ((signum == SIGINT) || (signum = SIGKILL)) {
        endwin();
        exit(EXIT_FAILURE);
    }
}

Response Client::SendRequest(RequestMsg& req){
    Response res = Response::SUCCESS;

    if (send(sockfd, &req, sizeof(req), 0) < 0) 
        goto end;
    if (read(sockfd, &res, sizeof(res)) < 0) 
        goto end;
    return res;

end:
    endwin();
    exit(EXIT_FAILURE);
}

void Client::Recv() {
    constexpr int BUFFER_SIZE = 256;
    char recv_buff[BUFFER_SIZE];

    display->InitMsgWin();
    while (1) {
        memset(recv_buff, 0, sizeof(recv_buff));
        int bytes = read(sockfd, recv_buff, sizeof(recv_buff));
        if (bytes <= 0) 
            goto end;
        
        std::unique_lock<std::mutex> lock(msg_queue_mutex);
        if (msg_queue.size() == MAX_MSG_DISPLAY) 
            msg_queue.pop_back();
      
        msg_queue.push_front(std::string(recv_buff, bytes - 1));
        lock.unlock();
        display->ShowMessage(msg_queue);
    }

end:
    display->closeWin();
}

void Client::Send() {
    constexpr int MSG_SIZE = 256;
    char input_message[MSG_SIZE];

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    display->InitInputWin();
    while (1) {
        int bytes = wgetnstr(display->InputWin, input_message, MSG_SIZE - 1);
        if (send(sockfd, input_message, strlen(input_message) + 1, 0) < 0) 
            goto end;

        std::unique_lock<std::mutex> lock(msg_queue_mutex);
        if (msg_queue.size() == MAX_MSG_DISPLAY) 
            msg_queue.pop_back();
        msg_queue.push_front(std::format("You: {}", std::string(input_message)));
        lock.unlock();
        display->ShowMessage(msg_queue);
        display->RefreshInputWin();
    }

end:
    display->closeWin();
}

void Client::Init() {
    this->sockfd = socket(AF_INET, this->type, 0);
    if (this->sockfd < 0) {
        perror("socket failed");
    }
    std::cout << "Client: create socket done\r\n";
    this->connection_timeout = 5;

    signal(SIGINT, SignalHandler);  // Handle Ctrl+C
    signal(SIGTERM, SignalHandler); // Handle termination signal
}

bool Client::Connect(const char *__addr) {
    this->addr.sin_family = AF_INET;
    this->addr.sin_addr.s_addr = inet_addr(__addr);
    this->addr.sin_port = htons(this->port);

    while (this->connection_timeout-- > 0) {
        int ret = connect(this->sockfd, (struct sockaddr *)&this->addr, sizeof(this->addr));
        if (ret == 0) {
            return 1;
        }
        sleep(1);    
    }
    std::cout << "Client: connection timeout 5s\r\n";
    return 0;
}

void Client::HandleConnection() {
    State state = SENREQUEST;
    bool SubThreadCreated = false;
    Response res;
    std::thread SendThread;
    std::thread ReceiveThread;

    printw("INFO: Connected to the Server\n\n");

    while (1) {
        switch (state) {
            case SENREQUEST: {
                RequestMsg reqmsg;
                char InputOpt[100];

                printw("1. Login\n2. Register\n3. Exit\n\nEnter your Option: ");
                refresh();

                getstr(InputOpt);
                reqmsg.option = std::stoi(InputOpt);

                if (reqmsg.option == 3) goto end;

                printw("\nEnter User Name: ");
                getstr(reqmsg.name);
                printw("Enter Password: ");
                getstr(reqmsg.pass);
                
                res = SendRequest(reqmsg);
                clear();
                switch (res) {
                    case SUCCESS:
                        printw("INFO: User %s Successful\n\n", reqmsg.option == 1 ? "Login" : "Registration");
                        state = SELECTCHATOPT;
                        break;

                    case ERROR_DUPLICATE_USERNAME:
                        printw("INFO: User Registration Failed (Duplicate User Name)\n\n");
                        break;

                    case ERROR_USERNAME_NOT_FOUND: 
                        printw("INFO: User Log In Failed (User Name Not Found)\n\n");
                        break;

                    case ERROR_PASSWORD_NOT_MATCHING:
                        printw("INFO: User Log In Failed (Password Not Matching)\n\n");
                        break;

                    case ERROR_USER_LOGGED_IN:
                        printw("INFO: User Log In Failed (User Already Logged In)\n\n");
                        break;
                    
                    case ERROR_UNKOWN:
                        printw("INFO: Unknown Error\n\n");

                    default:
                        break;
                }
                refresh();
                break;
            }
            
            case SELECTCHATOPT: {
                char InputOpt[100];
                uint8_t option;

                printw("Select Chat option:\n\n1: Single User Chat\n2: Multi User Chat\n3: Exit\n\nSelect what you would like to proceed with: ");
                refresh();
                getstr(InputOpt);
                option = std::stoi(InputOpt);

                if ((send(sockfd, &option, sizeof(option), 0) < 0) || (option > 3)) 
                    goto end;
                if (read(sockfd, &res, sizeof(res)) < 0) 
                    goto end;
                state = ONLINE;
                break;
            }

            case ONLINE: {
                if (!SubThreadCreated) {
                    ReceiveThread = std::thread(&Client::Recv, this);
                    SendThread = std::thread(&Client::Send, this);
                    SubThreadCreated = true;
                }
                break;
            }

            default:
                break;
        }
    }
    SendThread.join();
    ReceiveThread.join();

end:
    endwin();
    exit(EXIT_FAILURE);    
}

Client::Display::Display() {
    initscr();
    cbreak();
    echo();
    curs_set(0);
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);   // pair 1: red text, black background
    init_pair(2, COLOR_GREEN, COLOR_BLACK); // pair 2: green text, black background
}

void Client::Display::ShowMessage(std::deque<std::string>& msg_queue) {
    int row = 0;
    for (const auto& msg : msg_queue) {
        wmove(MsgWin, MAX_MSG_DISPLAY - row, 1);  
        wclrtoeol(MsgWin);  
        if (std::regex_search(msg, std::regex("INFO:"))) {
            if (std::regex_search(msg, std::regex("online"))) {
                wattron(MsgWin, COLOR_PAIR(2));
                mvwprintw(MsgWin, MAX_MSG_DISPLAY - row, 1, "%s", msg.c_str());
                wattroff(MsgWin, COLOR_PAIR(2));
            } else if (std::regex_search(msg, std::regex("offline"))) {
                wattron(MsgWin, COLOR_PAIR(1));
                mvwprintw(MsgWin, MAX_MSG_DISPLAY - row, 1, "%s", msg.c_str());
                wattroff(MsgWin, COLOR_PAIR(1));
            }
        } else {
            mvwprintw(MsgWin, MAX_MSG_DISPLAY - row, 1, "%s", msg.c_str());
        }
        row++;
    }        
    wrefresh(MsgWin);
}