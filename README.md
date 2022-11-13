# TCPServer

Description
- Handles up to 6 TCP clients
- New thread spawned for each new client
- Unique id created every second and sent to client
- Prints number of clients connecting when receiving new line character
- "Ctrl C" to exit server

How to compile
- MacOS: g++ -std=c++17 server.cpp -o server
- Linux: g++ -Wall -std=c++17 server.cpp -o server -lpthread