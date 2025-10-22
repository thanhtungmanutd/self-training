#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <iostream>
#include <format>
#include <string>
#include <SQLiteCpp/SQLiteCpp.h>

class MyDatabase {
    friend class Server;
    private:
        SQLite::Database db;
    
    public:
        MyDatabase(std::string&& __path) : db(__path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {};
        bool QueryUserInfo(const std::string& name, std::string& pass, int& status, int& sockfd);
        bool UpdateUserStatus(const std::string& name, int&& status, int&& sockfd);
        bool AddNewUserToDatabase(const std::string& name, const std::string& pass, int&& status, int& sockfd);
};

#endif /* _DATABASE_H_ */