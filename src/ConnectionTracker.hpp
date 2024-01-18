#pragma once
#include <cstdint>
#include <string>
#include <array>
#include <algorithm>
#include <memory>
#include <mutex>
#include <sys/types.h>
#include <system_error>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include "Address.hpp"

struct ConnectionEvent {
    int pid;
    std::array<char, 16> comm;
    short sock_type;
    std::unique_ptr<ConnectionTarget> target;
    uint64_t timestamp;
    std::vector<std::string> cmdline;

    ConnectionEvent(int pid, std::array<char, 16> comm, 
        short sock_type, std::unique_ptr<ConnectionTarget> target, uint64_t timestamp, 
        std::vector<std::string> cmdline): 
            pid(pid), comm(comm), sock_type(sock_type), target(target->clone()), timestamp(timestamp),
        cmdline(cmdline) {
    }

    ConnectionEvent(const ConnectionEvent& event): 
        pid(event.pid), comm(event.comm), sock_type(event.sock_type), cmdline(event.cmdline) {
        target = event.target->clone();
    }
};

class ConnectionSubscriber {
public:
    virtual void notify(ConnectionEvent) = 0;
};

class ConnectionTracker {
public:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void subscribe(std::shared_ptr<ConnectionSubscriber>) = 0;
    virtual void unsubscribe(std::shared_ptr<ConnectionSubscriber>) = 0;
};

class ConnectionTrackerBase: public ConnectionTracker {
    std::vector<std::shared_ptr<ConnectionSubscriber>> subscribers{};
    std::mutex mutex{};

protected:
    void publish(ConnectionEvent event);

public:
    virtual void subscribe(std::shared_ptr<ConnectionSubscriber> subscriber);
    virtual void unsubscribe(std::shared_ptr<ConnectionSubscriber> subscriber);
};
