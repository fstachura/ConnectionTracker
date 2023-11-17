#include <exception>
#include <array>
#include <stdio.h>
#include <string>
#include <system_error>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <bpf/libbpf.h>
#include "Address.hpp"
#include "bpf/trackers.skel.h"
#include "bpf/trackers.h"
#include "ConnectionTracker.hpp"
#include "BPFConnectionTracker.hpp"

BPFError::BPFError(int ec, std::string what_arg): std::system_error(ec, std::generic_category(), what_arg) {
}

int BPFConnectionTracker::ring_buffer_callback(void* tracker, void* data, size_t data_sz) {
    return static_cast<BPFConnectionTracker*>(tracker)->handle_event(data, data_sz);
}

int BPFConnectionTracker::handle_event(void* data, size_t data_sz) {
    connection_event* e = (struct connection_event*)malloc(data_sz);
    memcpy(e, data, data_sz);

    std::unique_ptr<ConnectionTarget> target;
    short port;

    if(e->sockaddr.sa_family == AF_INET6) {
        struct sockaddr_in6* in6 = (struct sockaddr_in6*)&e->sockaddr;
        std::array<std::byte, 16> addr;
        std::copy(reinterpret_cast<std::byte*>(&in6->sin6_addr), 
                reinterpret_cast<std::byte*>(&in6->sin6_addr)+16, addr.begin());
        target = std::make_unique<IPv6ConnecionTarget>(IPv6Address(addr), ntohs(in6->sin6_port));
    } else if(e->sockaddr.sa_family == AF_INET) {
        struct sockaddr_in* in4 = (struct sockaddr_in*)&e->sockaddr;
        std::array<std::byte, 4> addr;
        std::copy(reinterpret_cast<std::byte*>(&in4->sin_addr), 
                reinterpret_cast<std::byte*>(&in4->sin_addr)+4, addr.begin());
        target = std::make_unique<IPv4ConnecionTarget>(IPv4Address(addr), ntohs(in4->sin_port));
    } else if(e->sockaddr.sa_family == AF_LOCAL) {
        struct sockaddr_un* un = (struct sockaddr_un*)&e->sockaddr;
        target = std::make_unique<UnixConnectionTarget>(UnixAddress(un->sun_path));
    } else {
        size_t len =  data_sz - sizeof(connection_event);
        std::vector<std::byte> result(len);
        std::transform(e->sockaddr.sa_data, e->sockaddr.sa_data+len, result.begin(), 
                [](char c){ return static_cast<std::byte>(c); });
        target = std::make_unique<UnknownConnectionTarget>(UnknownAddress(e->sockaddr.sa_family, result));
    }

    std::array<char, 16> comm;
    std::copy(e->comm, e->comm+16, comm.begin());
    ConnectionEvent event(e->pid, comm, target->clone());

    publish(event);

    free(e);
    return 0;
}

void BPFConnectionTracker::start() {
    struct trackers_bpf* skel = trackers_bpf__open_and_load();
    if(!skel) {
        throw BPFError(errno, "failed to open and load bpf skeleton");
    }

    int err = trackers_bpf__attach(skel);
    if(err) {
        throw BPFError(err, "failed to attach bpf skeleton");
    }

    struct ring_buffer *rb = ring_buffer__new(
        bpf_map__fd(skel->maps.rb),
        ring_buffer_callback,
        this,
        NULL
    );

    while (!quit) {
        err = ring_buffer__poll(rb, 100);
        if (err == -EINTR) {
            err = 0;
            break;
        } else if (err < 0) {
            throw BPFError(err, "error polling for event");
            break;
        }
        sleep(1);
    }

    ring_buffer__free(rb);
    trackers_bpf__destroy(skel);
}

void BPFConnectionTracker::stop() {
    quit = true;
}
