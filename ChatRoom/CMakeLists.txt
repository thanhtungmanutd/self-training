cmake_minimum_required(VERSION 3.28)
project(ChatRoom)

find_package(Curses REQUIRED)

include_directories(${CURSES_INCLUDE_DIR})

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(SQLiteCpp)

include_directories(
    ${CMAKE_SOURCE_DIR}/lib/inc
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/SQLiteCpp/include
)

file(GLOB SERVER_SOURCES lib/src/socket_server.cpp lib/src/database.cpp src/server.cpp)
file(GLOB CLIENT_SOURCES lib/src/socket_client.cpp src/client.cpp)

add_executable(Client ${CLIENT_SOURCES})
target_link_libraries(Client ${CURSES_LIBRARIES})

add_executable(Server ${SERVER_SOURCES})
target_link_libraries(Server SQLiteCpp sqlite3)

