#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <string>
#include <SQLiteCpp/SQLiteCpp.h>

class MyDatabase {
    private:
        // SQLite::Database dtb;
    
    public:
        MyDatabase(std::string&& __path);
    //     bool QuerryUserInfo(std::string&& name, std::string& pass, int& status, int& sockfd);
    //     bool UpdateUserStatus(std::string&& name, int&& status, int&& sockfd);
    //     bool AddNewUserToDatabase(std::string&& name, std::string&& pass, int&& status, int& sockfd);
};

#endif /* _DATABASE_H_ */