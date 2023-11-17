#pragma once
#include <system_error>
#include "ConnectionTracker.hpp"
#include "BPFConnectionTracker.hpp"

class BPFError: public std::system_error {
public:
    BPFError(int ec, std::string what_arg);
};

class BPFConnectionTracker: public ConnectionTrackerBase {
    bool quit = false;
    static int ring_buffer_callback(void* tracker, void* data, size_t data_sz);
    int handle_event(void* data, size_t data_sz);

public:
    virtual void start();
    virtual void stop();
};
