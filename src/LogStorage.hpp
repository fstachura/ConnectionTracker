#include "ConnectionTracker.hpp"

class LogStorage {
public:
    virtual void logConnection(ConnectionEvent event) = 0;
    //virtual void getStats() = 0;
};
