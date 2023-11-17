#pragma once
#include <algorithm>
#include <string>
#include <memory>
#include <array>
#include <system_error>
#include <vector>
#include <arpa/inet.h>

class Address {
public:
    virtual std::string to_string() = 0;
    virtual std::vector<std::byte> to_bytes() = 0;
    virtual std::unique_ptr<Address> clone() = 0;
    virtual ~Address() {}
};

class ConnectionTarget {
public:
    virtual std::string to_string() = 0;
    virtual std::unique_ptr<ConnectionTarget> clone() = 0;
    virtual ~ConnectionTarget() {}
};

template<unsigned int F, size_t BL, size_t SL>
class IPAddress: public Address {
    std::array<std::byte, BL> data;
public:
    IPAddress(std::array<std::byte, BL> data): data(data) {}
    virtual ~IPAddress() {}

    virtual std::string to_string() {
        char addr[SL] = {0};
        const char* err = inet_ntop(F, data.data(), addr, sizeof(addr));
        if(err == NULL) {
            throw std::system_error(errno, std::generic_category(), "failed to convert ip address to string");
        }
        addr[SL-1] = 0;
        return std::string(addr);
    }

    virtual std::vector<std::byte> to_bytes() {
        return std::vector<std::byte>(data.begin(), data.end());
    }

    virtual std::unique_ptr<Address> clone() {
        return std::make_unique<IPAddress>(data);
    }
};

typedef IPAddress<AF_INET, 4, INET_ADDRSTRLEN> IPv4Address;
typedef IPAddress<AF_INET6, 16, INET6_ADDRSTRLEN> IPv6Address;

class UnixAddress: public Address {
    std::string data;
public:
    UnixAddress(std::string data);
    virtual ~UnixAddress();
    virtual std::string to_string();
    virtual std::vector<std::byte> to_bytes();
    virtual std::unique_ptr<Address> clone();
};

class UnknownAddress: public Address {
    std::vector<std::byte> data;
    sa_family_t family;
public:
    UnknownAddress(sa_family_t family, std::vector<std::byte> data);
    virtual ~UnknownAddress();
    virtual std::string to_string();
    virtual std::vector<std::byte> to_bytes();
    virtual std::unique_ptr<Address> clone();
};

template<typename T>
class IPConnectionTarget: public ConnectionTarget {
    T address;
    uint16_t port;
public:
    IPConnectionTarget(T address, uint16_t port): address(address), port(port) {}
    virtual ~IPConnectionTarget() {}

    virtual std::string to_string() {
        return address.to_string() + ":" + std::to_string(port);
    }

    virtual std::unique_ptr<ConnectionTarget> clone() {
        return std::make_unique<IPConnectionTarget>(address, port);
    }
};

typedef IPConnectionTarget<IPv4Address> IPv4ConnecionTarget;
typedef IPConnectionTarget<IPv6Address> IPv6ConnecionTarget;

template<typename T>
class GenericConnectionTarget: public ConnectionTarget {
    T address;
public:
    GenericConnectionTarget(T address): address(address) {}
    virtual ~GenericConnectionTarget() {}

    virtual std::string to_string() {
        return address.to_string();
    }

    virtual std::unique_ptr<ConnectionTarget> clone() {
        return std::make_unique<GenericConnectionTarget<T>>(address);
    }
};

typedef GenericConnectionTarget<UnixAddress> UnixConnectionTarget;
typedef GenericConnectionTarget<UnknownAddress> UnknownConnectionTarget;
