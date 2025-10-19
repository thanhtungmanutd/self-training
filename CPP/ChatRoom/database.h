#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <iostream>
#include <string>
#include <cstdio>
#include <sqlite3.h>

class Database {
    private:
        sqlite3 *database;
    
    public:
        Database(std::string&& databbase_path);
        sqlite3* SQLite3() {return this->database;}
        bool QuerryUserInfo(std::string&& name, std::string& pass, int& status, int& sockfd);
        bool UpdateUserStatus(std::string&& name, int&& status, int&& sockfd);
        bool AddNewUserToDatabase(std::string&& name, std::string&& pass, int&& status, int& sockfd);
};

#endif /* _DATABASE_H_ */