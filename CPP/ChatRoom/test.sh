#!/bin/bash

PORT="80"
if [ -n "$1" ]; then
    PORT="$1"
fi

# build client
echo "Compiling source"
mkdir -p build
cd build
cmake .. 
make clean && make Client
cp Client ..
rm -rf Client

# running client
echo "Using port number: $PORT"
cd ..

# log in and auto send msg periodically
expect auto_login.exp "$PORT"
