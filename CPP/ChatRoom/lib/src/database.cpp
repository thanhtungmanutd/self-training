#include "database.h"

MyDatabase::MyDatabase(std::string&& __path) {
//     // if (sqlite3_open(database_path.c_str(), &database) != SQLITE_OK) 
//     //     cout << "Chat Server: Failed to connect to databse\r\n"; 
}

// bool Database::QuerryUserInfo(string&& name,
//                               string& pass,
//                               int& status,
//                               int& sockfd) {
//     // string cmd = "SELECT * FROM usrs WHERE username = '" + name + "';";
//     // sqlite3_stmt *stmt;

//     // if (sqlite3_prepare_v2(database, cmd.c_str(), -1, &stmt, 0) != SQLITE_OK) {
//     //     cout << "Chat Server: Failed to prepare statement to querry data\r\n";
//     //     return false;
//     // }

//     // if (sqlite3_step(stmt) == SQLITE_ROW) {
//     //     pass = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))); 
//     //     status = sqlite3_column_int(stmt, 2);
//     //     sockfd = sqlite3_column_int(stmt, 3);
//     //     sqlite3_finalize(stmt);
//     //     return true;
//     // }
//     // sqlite3_finalize(stmt);
//     return false;
// }

// bool Database::UpdateUserStatus(string&& name, int&& status, int&& sockfd) {
//     // string cmd = "UPDATE usrs SET status = ?, sockfd = ? WHERE username = ?;";
//     // sqlite3_stmt *stmt;

//     // if (sqlite3_prepare_v2(database, cmd.c_str(), -1, &stmt, 0)) {
//     //     cout << "Chat Server: Failed to prepare statement to update data\r\n";
//     //     return false;
//     // }

//     // sqlite3_bind_int(stmt, 1, status);
//     // sqlite3_bind_int(stmt, 2, sockfd);
//     // sqlite3_bind_text(stmt, 3, name.c_str(), -1, SQLITE_STATIC);

//     // if (sqlite3_step(stmt) == SQLITE_DONE) {
//     //     sqlite3_finalize(stmt);
//     //     return true;
//     // }
//     // sqlite3_finalize(stmt);
//     return false;
// }

// bool Database::AddNewUserToDatabase(string&& name,
//                                     string&& pass,
//                                     int&& status,
//                                     int& sockfd) {
//     // char cmd[1000] = {0};

//     // sprintf(cmd, "INSERT INTO usrs (username, password, status, sockfd) VALUES ('%s', '%s', %d, %d);",
//     //         name.c_str(), pass.c_str(), status, sockfd);
//     // if (sqlite3_exec(database, cmd, 0, 0, NULL) == SQLITE_OK) {
//     //     return true;
//     // }
//     // return false;
// }