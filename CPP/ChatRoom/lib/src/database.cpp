#include "database.h"

bool MyDatabase::QueryUserInfo(const std::string& name, std::string& pass, int& status, int& sockfd) {
    SQLite::Statement query(this->db, std::format("SELECT * FROM usrs WHERE username = '{}';", name));

    if (query.executeStep()) {
        pass = query.getColumn(1).getString(); // first column
        status = query.getColumn(2).getInt();  // second column
        sockfd = query.getColumn(3).getInt();  // third column
        return true;
    } else {
        return false;
    }
    return false;
}

bool MyDatabase::UpdateUserStatus(const std::string& name, int&& status, int&& sockfd) {
    SQLite::Statement query(this->db, std::format("UPDATE usrs SET status = {}, sockfd = {} WHERE username = '{}';", status, sockfd, name));
    try {
        query.exec();
    } catch(const SQLite::Exception& e) {
        return false;
    }
    return true;
}

bool MyDatabase::AddNewUserToDatabase(const std::string& name, const std::string& pass, int&& status, int& sockfd) {
    SQLite::Statement query(this->db, std::format("INSERT INTO usrs (username, password, status, sockfd) VALUES ('{}', '{}', {}, {});", name, pass, status, sockfd));
    try {
        query.exec();
    } catch(const SQLite::Exception& e) {
        return false;
    }
    return true;
}