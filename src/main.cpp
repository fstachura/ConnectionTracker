#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <liburing.h>
#include "BPFConnectionTracker.hpp"
#include "Address.hpp"
#include "ConnectionTracker.hpp"
#include "LogStorage.hpp"
#include "SQLite.hpp"
#include "SQLiteLogStorage.hpp"
#include "Socket.hpp"

typedef unsigned long long u64;

class StdoutConnectionSubsciber: public ConnectionSubscriber {
    virtual void notify(ConnectionEvent event) {
        std::string comm(event.comm.begin(), event.comm.end());
        auto t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);
        std::cout<<"["<<std::put_time(&tm, "%F %T")<<"] "<<comm<<" "<<event.pid<<" (";
        for(auto& c: event.cmdline) {
            std::cout<<c;
            if(c != *(event.cmdline.end()-1)) {
                std::cout<<" ";
            }
        }
        std::cout<<") "<<event.target->to_string()<<" ("<<sock_type_to_string(event.sock_type)<<")"<<std::endl;
    }
};

class LogStorageConnectionSubscriber: public ConnectionSubscriber {
    std::unique_ptr<LogStorage> storage;
public:
    LogStorageConnectionSubscriber(std::unique_ptr<LogStorage> storage): storage(std::move(storage)) {}

    virtual void notify(ConnectionEvent event) {
        while(true) {
            try {
                storage->logConnection(event);
                return;
            } catch(SQLiteException e) {
                if(e.getErrcode() == SQLITE_LOCKED) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                } 
                throw e;
            }
        }
    }
};

int main() {
    auto storage = std::make_unique<SQLiteLogStorage>("/tmp/connectiontracker.sqlite");
    auto storageSubscriber = std::make_shared<LogStorageConnectionSubscriber>(std::move(storage));

    //Socket socket;
    //socket.start();

    auto tracker = BPFConnectionTracker();
    tracker.subscribe(std::make_shared<StdoutConnectionSubsciber>());
    tracker.subscribe(storageSubscriber);
    tracker.start();
}
