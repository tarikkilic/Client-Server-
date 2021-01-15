# Client-Server-

https://github.com/tarikkilic/Client-Server/blob/main/REPORT.pdf
Just tested in Ubuntu.

Usage:

1. cd Client-Server
2. make
3. ./server -i <pathToFile> -p <PORT> -o <pathToLogFile> -s <4> -x <24>
  sample input is 'input.txt'. s is number of thread at the beggining. x is max number of thread.
  Serve running as a deamon. you can look with top command.
4. Usage: ./client -a 127.0.0.1 -p PORT -s 5 -d 7
  If you make  same request twice, you should recognize, request time is lower because of cache mechanism.
  s is start node. d is destination node.
