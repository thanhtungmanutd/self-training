#include "socket_client.h"

void Client::Init() {
    this->sockfd = socket(AF_INET, this->type, 0);
    if (this->sockfd < 0) {
        perror("socket failed");
    }
    std::cout << "Client: create socket done\r\n";
    this->connection_timeout = 5;

    signal(SIGINT, SignalHandler);  // Handle Ctrl+C
    signal(SIGTERM, SignalHandler); 
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

void Client::SignalHandler(int signum) {
    if ((signum == SIGINT) || (signum = SIGKILL)) 
        exit(EXIT_SUCCESS);
}

Response Client::SendRequest(RequestMsg& req){
    Response res = Response::SUCCESS;

    if (send(sockfd, &req, sizeof(req), 0) < 0) 
        std::cout << "Client: failed to send request message\r\n";

    if (read(sockfd, &res, sizeof(res)) < 0) 
        std::cout << "Client: failed to read response message\r\n";
    return res;
}

void Client::HandleConnection() {
    int option;
    std::string name;
    State state = SENREQUEST;
    bool SubThreadCreated = false;
    
    while (1) {
        switch (state) {
            case SENREQUEST:
                uint8_t opt;
                Response res;
                RequestMsg reqmsg;

                std::cout << "INFO: Connected to the Server\r\n\n";
                std::cout << "1. Login\r\n2. Register\r\n3. Exit\r\n\n";
                std::cout << "Enter your Option: " << std::flush;;
                std::cin >> reqmsg.option;

                if (reqmsg.option == 3) 
                    exit(EXIT_SUCCESS);

                std::cout << "\r\nEnter User Name: ";
                std::cin >> reqmsg.name;
                std::cout << "\rEnter Password: ";
                std::cin >> reqmsg.pass;

                res = SendRequest(reqmsg);
                switch (res) {
                    case SUCCESS:
                        if (reqmsg.option == '1')
                            std::cout << "\nINFO: User Login Successful\r\n\n";
                        if (reqmsg.option == '2')
                            std::cout << "\nINFO: User Registration Successful\r\n\n";
                        state = SELECTCHATOPT;
                        break;

                    case ERROR_DUPLICATE_USERNAME:
                        std::cout << "\nINFO: User Registration Failed (Duplicate User Name)\r\n\n";
                        break;

                    case ERROR_USERNAME_NOT_FOUND: 
                        std::cout << "\nINFO: User Log In Failed (User Name Not Found)\r\n\n";
                        break;

                    case ERROR_PASSWORD_NOT_MATCHING:
                        std::cout << "\nINFO: User Log In Failed (Password Not Matching)\r\n\n";
                        break;

                    case ERROR_USER_LOGGED_IN:
                        std::cout << "\nINFO: User Log In Failed (User Already Logged In)\r\n\n";
                        break;
                    
                    case ERROR_UNKOWN:
                        std::cout << "\nINFO: Unknown Error\r\n\n";

                    default:
                        break;
                }
                state = SELECTCHATOPT;
                break;
            
            case SELECTCHATOPT:
                std::cout << "Chat option:\n\r1: Single User Chat\r\n2: Multi User Chat\r\n3: Exit\r\n\n";
                std::cout << "Select what you would like to proceed with: ";
                std::cin >> option;

                if (send(sockfd, &option, sizeof(option), 0) < 0) {
                    std::cout << "Client: Server seems to crash close program\r\n";
                    exit(EXIT_SUCCESS);
                }
                if (option == 3) 
                    exit(EXIT_SUCCESS);
                if (read(sockfd, &res, sizeof(res)) < 0) {
                    std::cout << "Client: Server seems to crash close program\r\n";
                    exit(EXIT_SUCCESS);
                }
                state = ONLINE;
                break;

            case ONLINE:
                break;

            default:
                break;
        }
    }
}