#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "BPFConnectionTracker.hpp"
#include "Address.hpp"
#include "ConnectionTracker.hpp"

typedef unsigned long long u64;

class StdoutConnectionSubsciber: public ConnectionSubscriber {
    virtual void notify(ConnectionEvent event) {
        std::string comm(event.comm.begin(), event.comm.end());
        auto t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);
        std::cout<<"["<<std::put_time(&tm, "%F %T")<<"] "<<comm<<" "<<event.pid<<" "<<event.target->to_string()
            <<" ("<<sock_type_to_string(event.sock_type)<<")"<<std::endl;
    }
};

int main() {
    auto tracker = BPFConnectionTracker();
    tracker.subscribe(std::make_shared<StdoutConnectionSubsciber>());
    tracker.start();
}
