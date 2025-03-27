cmake_minimum_required(VERSION 3.28)
project(ChatRoom)
find_package(SQLite3 REQUIRED)
add_executable(Client client.cpp tcp_socket.cpp)
target_link_libraries(Client PRIVATE SQLite::SQLite3)
add_executable(Server server.cpp tcp_socket.cpp)
target_link_libraries(Server PRIVATE SQLite::SQLite3)

