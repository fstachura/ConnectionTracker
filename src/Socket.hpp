#pragma once
#include <memory>
#include <variant>
#include <liburing.h>
#include "IOUring.hpp"

const char SOCKET_DIR[] = "/var/run/connectiontracker/";
const char SOCKET_PATH[] = "/var/run/connectiontracker/socket";

class AcceptSocketRequest {
};

class WriteSocketRequest {
public:
    int fd;
    WriteSocketRequest(int fd): fd(fd) {
    }
};

class ReadSocketRequest {
public:
    int fd;
    ReadSocketRequest(int fd): fd(fd) {
    }
};

class CloseSocketRequest {
};

using SocketRequest = std::variant<AcceptSocketRequest, 
      WriteSocketRequest, ReadSocketRequest, CloseSocketRequest>;

class Socket {
    int socketFd;
    IOUring<SocketRequest> ring;

    void submitAccept();
    void submitWrite(int fd, std::string);
    void submitRead(int fd);
    void submitClose(int fd);
    void prepareFS();
    void prepareSocket();

public:
    Socket();
    void start();
};

// cmds:
// start
// stop
// exit
// stats
