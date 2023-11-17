#include <string>
#include <memory>
#include <arpa/inet.h>
#include "Address.hpp"

const std::string to_string(sa_family_t family) {
    if(family == AF_UNSPEC) {
        return "AF_UNSPEC";
    } else if(family == AF_LOCAL) {
        return "AF_LOCAL";
    } else if(family == AF_UNIX) {
        return "AF_UNIX";
    } else if(family == AF_FILE) {
        return "AF_FILE";
    } else if(family == AF_INET) {
        return "AF_INET";
    } else if(family == AF_AX25) {
        return "AF_AX25";
    } else if(family == AF_IPX) {
        return "AF_IPX";
    } else if(family == AF_APPLETALK) {
        return "AF_APPLETALK";
    } else if(family == AF_NETROM) {
        return "AF_NETROM";
    } else if(family == AF_BRIDGE) {
        return "AF_BRIDGE";
    } else if(family == AF_ATMPVC) {
        return "AF_ATMPVC";
    } else if(family == AF_X25) {
        return "AF_X25";
    } else if(family == AF_INET6) {
        return "AF_INET6";
    } else if(family == AF_ROSE) {
        return "AF_ROSE";
    } else if(family == AF_DECnet) {
        return "AF_DECnet";
    } else if(family == AF_NETBEUI) {
        return "AF_NETBEUI";
    } else if(family == AF_SECURITY) {
        return "AF_SECURITY";
    } else if(family == AF_KEY) {
        return "AF_KEY";
    } else if(family == AF_NETLINK) {
        return "AF_NETLINK";
    } else if(family == AF_ROUTE) {
        return "AF_ROUTE";
    } else if(family == AF_PACKET) {
        return "AF_PACKET";
    } else if(family == AF_ASH) {
        return "AF_ASH";
    } else if(family == AF_ECONET) {
        return "AF_ECONET";
    } else if(family == AF_ATMSVC) {
        return "AF_ATMSVC";
    } else if(family == AF_RDS) {
        return "AF_RDS";
    } else if(family == AF_SNA) {
        return "AF_SNA";
    } else if(family == AF_IRDA) {
        return "AF_IRDA";
    } else if(family == AF_PPPOX) {
        return "AF_PPPOX";
    } else if(family == AF_WANPIPE) {
        return "AF_WANPIPE";
    } else if(family == AF_LLC) {
        return "AF_LLC";
    } else if(family == AF_IB) {
        return "AF_IB";
    } else if(family == AF_MPLS) {
        return "AF_MPLS";
    } else if(family == AF_CAN) {
        return "AF_CAN";
    } else if(family == AF_TIPC) {
        return "AF_TIPC";
    } else if(family == AF_BLUETOOTH) {
        return "AF_BLUETOOTH";
    } else if(family == AF_IUCV) {
        return "AF_IUCV";
    } else if(family == AF_RXRPC) {
        return "AF_RXRPC";
    } else if(family == AF_ISDN) {
        return "AF_ISDN";
    } else if(family == AF_PHONET) {
        return "AF_PHONET";
    } else if(family == AF_IEEE802154) {
        return "F_IEEE802154";
    } else if(family == AF_CAIF) {
        return "AF_CAIF";
    } else if(family == AF_ALG) {
        return "AF_ALG";
    } else if(family == AF_NFC) {
        return "AF_NFC";
    } else if(family == AF_VSOCK) {
        return "AF_VSOCK";
    } else if(family == AF_KCM) {
        return "AF_KCM";
    } else if(family == AF_QIPCRTR) {
        return "AF_QIPCRTR";
    } else if(family == AF_SMC) {
        return "AF_SMC";
    } else if(family == AF_XDP) {
        return "AF_XDP";
    } else if(family == AF_MCTP) {
        return "AF_MCTP";
    } else if(family == AF_MAX) {
        return "AF_MAX";
    } else {
        return "UNKNOWN";
    }
}

UnixAddress::UnixAddress(std::string data): data(data) {}

UnixAddress::~UnixAddress() {}

std::string UnixAddress::to_string() {
    return data;
}

std::vector<std::byte> UnixAddress::to_bytes() {
    std::vector<std::byte> result(data.size());
    std::transform(data.begin(), data.end(), result.begin(), 
            [](char c){ return static_cast<std::byte>(c); });
    return result;
}

std::unique_ptr<Address> UnixAddress::clone() {
    return std::make_unique<UnixAddress>(data);
}

UnknownAddress::UnknownAddress(sa_family_t family, std::vector<std::byte> data): family(family), data(data) {}

UnknownAddress::~UnknownAddress() {}

std::string UnknownAddress::to_string() {
    return "unknown (" + ::to_string(family) + ")";
}

std::vector<std::byte> UnknownAddress::to_bytes() {
    return data;
}

std::unique_ptr<Address> UnknownAddress::clone() {
    return std::make_unique<UnknownAddress>(family, data);
}
