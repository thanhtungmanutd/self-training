#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <iostream>
#include <memory>
#include <deque>
#include <cstring>
#include <format>
#include <thread>
#include <csignal>
#include <unistd.h>
#include <regex>
#include <mutex>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ncurses.h>

#define MAX_MSG_DISPLAY                 20

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
    class Display {
        public:
            WINDOW *MsgWin;
            WINDOW *InputWin;
            Display();
            void InitMsgWin() {
                clear();
                refresh();
                MsgWin = newwin(MAX_MSG_DISPLAY + 1, getmaxx(stdscr), 0, 0);
                wrefresh(MsgWin);
            };

            void InitInputWin() {
                InputWin = newwin(2, getmaxx(stdscr), MAX_MSG_DISPLAY + 2, 0);
                mvwhline(InputWin, 0, 0, ACS_HLINE, getmaxx(stdscr));
                mvwprintw(InputWin, 1, 1, "> ");
                wrefresh(InputWin);
            };

            void RefreshInputWin() {
                wclear(this->InputWin);
                mvwhline(InputWin, 0, 0, ACS_HLINE, getmaxx(stdscr));
                mvwprintw(InputWin, 1, 1, "> ");
                wrefresh(this->InputWin);
            };

            void closeWin() {
                if (this->MsgWin) 
                    delwin(this->MsgWin);
                if (this->InputWin)
                    delwin(this->InputWin);
                endwin();
                exit(EXIT_FAILURE);
            };

            ~Display() {endwin();};
            void ShowMessage(std::deque<std::string>& msg_queue);
    };

    private:
        enum State {SENREQUEST, SELECTCHATOPT, ONLINE};
        int port = 80;
        int type = SOCK_STREAM;
        int sockfd = -1;
        int connection_timeout = 5;
        struct sockaddr_in addr;
        std::deque<std::string> msg_queue;
        State state;
        std::mutex msg_queue_mutex;
        std::unique_ptr<Display> display = std::make_unique<Display>();

    private:
        static void SignalHandler(int signum);
        Response SendRequest(RequestMsg& reqmsg);
        void Recv();
        void Send();

    public:
        Client(int&& __port, int&& __type) : port(__port), type(__type) {};
        void Init();
        bool Connect(const char *addr);
        void HandleConnection();
        ~Client() {endwin();};
};
#endif /* _CLIENT_H_ */