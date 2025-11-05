# CHAT ROOM #

## Overview ##

This projects aims to simulate a chat application with minimal functionalities. The idea and requirements for this projects comes from [EMERTXE Linux Internals Projects for Beginners | Tcp/Ip Chat Application Using C](https://www.emertxe.com/embedded-systems/linux-internals/li-projects/tcp-ip-chat-room).

## Dependencies ##

- Cmake
- SQLite
- SQLiteCpp (C/C++ library for SQLite3)
- Ncurse

## Step to run ##

We need to install all the dependencies to be able to compile and run the project. There will be at least one server application for handling connections from other clients while you can have multiple clients connect to the server at the same time.

There is an automation test script in the autotest folder. You can run the client_autotest.sh the auto login then send message to the other users.
