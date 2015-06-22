#!/bin/bash

./hello_world_server &
# send a msg to ip:port  
echo -n "hello server" | nc 127.0.0.1 9051
sleep 10
killall hello_world_server
