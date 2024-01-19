#include <chrono>
#include <iostream>
#include <exception>
#include <memory>
#include <thread>
#include <system_error>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <liburing.h>
#include "Socket.hpp"

// my favorite snippets of code :)
// stolen from https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void Socket::submitAccept() {
    auto sqe = ring.getSqeBlocking();
    ring.prepareAccept(sqe, std::make_unique<SocketRequest>(AcceptSocketRequest()), socketFd, NULL, NULL, 0);
    ring.submit();
}

void Socket::submitWrite(int fd, std::string buf) {
    auto sqe = ring.getSqeBlocking();
    auto req = WriteSocketRequest(fd);
    auto size = buf.size();
    ring.prepareWrite(sqe, std::make_unique<SocketRequest>(std::move(req)), fd, std::move(buf), size, 0);
    ring.submit();
}

void Socket::submitRead(int fd) {
    auto sqe = ring.getSqeBlocking();
    std::string buf;
    buf.resize(256);
    auto req = ReadSocketRequest(fd);
    ring.prepareRead(sqe, std::make_unique<SocketRequest>(std::move(req)), fd, std::move(buf));
    ring.submit();
}

void Socket::submitClose(int fd) {
    auto sqe = ring.getSqeBlocking();
    ring.prepareClose(sqe, std::make_unique<SocketRequest>(CloseSocketRequest()), fd);
    ring.submit();
}

void Socket::prepareFS() {
    DIR* opendirResult = opendir(SOCKET_DIR);
    if(opendirResult == NULL) {
        if(errno == ENOENT) {
            int mkdirResult = mkdir(SOCKET_DIR, 0700);
            if(mkdirResult == -1) {
                throw std::system_error(errno, std::system_category(), "failed to create socket direcotory");
            }
        } else {
            throw std::system_error(errno, std::system_category(), "failed to check socket directory existence");
        }
    }

    // not the best option probably TODO handle signals, error out if socket exists
    int unlinkResult = unlink(SOCKET_PATH);
    if(unlinkResult == 1 && errno != ENOENT) {
        throw std::system_error(errno, std::system_category(), "failed to unlink existing socket");
    }
}

void Socket::prepareSocket() {
    socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(socketFd == -1) {
        throw std::system_error(errno, std::system_category(), "failed to open socket");
    }
    bool reuseaddrOpt = 1;
    int setsockoptResult = setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuseaddrOpt, sizeof(int));
    if(setsockoptResult == -1) {
        throw std::system_error(errno, std::system_category(), "failed to set socket options");
    }
    sockaddr_un socket_addr;
    memcpy(&socket_addr.sun_path, SOCKET_PATH, sizeof(SOCKET_PATH));
    socket_addr.sun_family = AF_UNIX;
    int bind_result = bind(socketFd, (struct sockaddr*)&socket_addr, sizeof(sockaddr_un));
    if(bind_result == -1) {
        throw std::system_error(errno, std::system_category(), "failed to bind socket");
    }
    int listen_result = listen(socketFd, 16);
    if(listen_result == -1) {
        throw std::system_error(errno, std::system_category(), "failed to listen on socket");
    }
}

Socket::Socket() {
    prepareFS();
    prepareSocket();
    submitAccept();
}

void Socket::start() {
    while(true) {
        IOUringCqe cqe = ring.waitCqe();
        auto ud = cqe.getUserData();
        std::visit(overloaded{
            [&](AcceptSocketRequest req) {
                if(cqe.res() < 0) {
                    std::cerr << "accept returned error " << std::system_category().message(-cqe.res()) << std::endl;
                } else {
                    std::cout << "received accept" << std::endl;
                    submitWrite(cqe.res(), "hello\n");
                }
                submitAccept();
            },
            [&](WriteSocketRequest req) {
                if(cqe.res() < 0) {
                    std::cerr << "write returned error " << std::system_category().message(-cqe.res()) << std::endl;
                    submitClose(req.fd);
                } else {
                    std::cout << "received write " << cqe.res() << std::endl;
                    submitRead(req.fd);
                }
            },
            [&](ReadSocketRequest req) {
                if(cqe.res() < 0) {
                    std::cerr << "read returned error " << std::system_category().message(-cqe.res()) << std::endl;
                    submitClose(req.fd);
                } else {
                    std::cout << "received read " << cqe.res() << std::endl;
                    submitWrite(req.fd, std::get<std::string>(cqe.getArg(0)));
                }
            },
            [&](CloseSocketRequest req) {
                if(cqe.res() < 0) {
                    std::cerr << "close returned error " << std::system_category().message(-cqe.res()) << std::endl;
                } else {
                    std::cout << "received close " << cqe.res() << std::endl;
                }
            },
        }, *ud);
    }
}
