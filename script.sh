#!/bin/sh
xterm -title "master 1" -e "./tsd -i localhost -c 9000 -p 10000 -d 1 -t master" &
xterm -title "master 2" -e "./tsd -i localhost -c 9000 -p 8080 -d 2 -t master" &
xterm -title "master 3" -e "./tsd -i localhost -c 9000 -p 7000 -d 3 -t master" &
xterm -title "slave 1" -e "./tsd -i localhost -c 9000 -p 9100 -d 1 -t slave" &
xterm -title "slave 2" -e "./tsd -i localhost -c 9000 -p 9200 -d 2 -t slave" &
xterm -title "slave 3" -e "./tsd -i localhost -c 9000 -p 9300 -d 3 -t slave" 

