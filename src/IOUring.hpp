#pragma once
#include <memory>
#include <iostream>
#include <functional>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <thread>
#include <system_error>
#include <variant>
#include <liburing.h>

using Arg = std::variant<std::vector<std::byte>, std::string>;

template<typename T>
class IOUringCqe;

template<typename T>
class IOUringSqe;

template<typename T>
class IOUringUserData;

template<typename T>
class IOUring {
    std::shared_ptr<io_uring> ring;
    int io_fd;

    void prepareSqe(IOUringSqe<T> sqe, IOUringUserData<T>* userData) {
        if(sqe.ring != ring) {
            throw std::runtime_error("sqe belongs to a different ring");
        } else if(sqe.prepared) {
            throw std::runtime_error("sqe already prepared");
        }
        io_uring_sqe_set_data(sqe.sqe, userData);
        sqe.prepared = true;
    }

public:
    template<typename... Args>
    class prepare {
        void (*f)(io_uring_sqe*, Args...);
        IOUring<T>& ring;

    public:
        prepare(IOUring<T>& ring, void f(io_uring_sqe*, Args...)): f(f), ring(ring) {
        }

        prepare(prepare const&) = delete;
        prepare(prepare &&) = delete;
        prepare& operator=(prepare const&) = delete;
    
        void operator()(IOUringSqe<T> sqe, std::unique_ptr<T> userData, Args... args) const {
            auto ud = new IOUringUserData(std::move(userData));
            ring.prepareSqe(sqe, ud);
            f((io_uring_sqe*)sqe.sqe, args...);
        }
    }; 

    const prepare<int, sockaddr*, socklen_t*, int> prepareAccept;
    const prepare<int> prepareClose;

    IOUring(): 
            prepareAccept(*this, &io_uring_prep_accept),
            prepareClose(*this, &io_uring_prep_close) {
        io_uring* ring_tmp; 
        io_fd = io_uring_queue_init(16, ring_tmp, IORING_SETUP_SQPOLL);
        ring.reset(ring_tmp, io_uring_queue_exit);
        if(io_fd == -1) {
            throw std::system_error(errno, std::system_category(), "failed to create io_uring");
        }
    }
    IOUring(const IOUring& r) = delete;
    IOUring operator=(const IOUring&) = delete;
    IOUring(const IOUring<T>&& r): ring(r.ring), io_fd(r.io_fd) {
    }

    IOUringSqe<T> getSqeBlocking() {
        while(true) {
            io_uring_sqe* sqe = io_uring_get_sqe(ring.get());
            if(sqe != NULL) {
                return IOUringSqe<T>(ring, sqe);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    IOUringCqe<T> waitCqe() {
        io_uring_cqe* cqe;
        int result = io_uring_wait_cqe(ring.get(), &cqe);
        if(result < 0) {
            throw std::system_error(-result, std::system_category(), "failed to wait for cqe");
        }
        return IOUringCqe<T>(ring, cqe);
    }

    void submit() {
        int result = io_uring_submit(ring.get());
        if(result < 0) {
            throw std::system_error(-result, std::system_category(), "failed to submit io_uring");
        }
    }

    template<typename B>
    void prepareWrite(IOUringSqe<T> sqe, std::unique_ptr<T> userData, int fd, B buf) {
        auto size = buf.size();
        prepareWrite(sqe, userData, fd, std::move(buf), size, 0);
    }

    template<typename B>
    void prepareWrite(IOUringSqe<T> sqe, std::unique_ptr<T> userData, 
            int fd, B buf, unsigned int size, __u64 offset) {
        if(size > buf.size()) {
            throw std::runtime_error("prepareWrite: size > buf.size()");
        }
        if(offset >= size) {
            throw std::runtime_error("prepareWrite: offset >= size");
        }

        std::vector args = {
            std::variant<std::vector<std::byte>, std::string>(std::move(buf))
        };
        auto ud = new IOUringUserData<T>(
            std::move(userData),
            std::move(args)
        );
        prepareSqe(sqe, ud);
        auto udbuf = std::get<B>(ud->args().at(0)).data();
        io_uring_prep_write(sqe.sqe, fd, udbuf, size, offset);
    }

    template<typename B>
    void prepareRead(IOUringSqe<T> sqe, std::unique_ptr<T> userData, int fd, B buf) {
        auto size = buf.size();
        prepareRead(sqe, std::move(userData), fd, std::move(buf), size, 0);
    }

    template<typename B>
    void prepareRead(IOUringSqe<T> sqe, std::unique_ptr<T> userData, 
            int fd, B buf, unsigned int size, __u64 offset) {
        if(size > buf.size()-offset) {
            throw std::runtime_error("prepareRead: size >= buf.size()-offset");
        }
        if(offset >= size) {
            throw std::runtime_error("prepareReadd: offset >= size");
        }

        std::vector args = {
            std::variant<std::vector<std::byte>, std::string>(std::move(buf))
        };
        auto ud = new IOUringUserData<T>(
            std::move(userData),
            std::move(args)
        );
        prepareSqe(sqe, ud);
        auto udbuf = std::get<B>(ud->args().at(0)).data();
        io_uring_prep_read(sqe.sqe, fd, udbuf, size, offset);
    }

    ~IOUring() {
    }
};

template<typename T>
class IOUringUserData {
    std::unique_ptr<T> userData;
    std::vector<Arg> persistentArgs;
public:
    IOUringUserData(std::unique_ptr<T>&& userData): userData(std::move(userData)) {
    }

    IOUringUserData(std::unique_ptr<T>&& userData, std::vector<Arg>&& persistentArgs): 
        userData(std::move(userData)), persistentArgs(std::move(persistentArgs)) {
    }

    std::vector<Arg>& args() {
        return persistentArgs;
    }

    std::unique_ptr<T>&& getUserData() {
        return std::move(userData);
    }
};

template<typename T>
class IOUringSqe {
    std::shared_ptr<io_uring> ring;
    io_uring_sqe* sqe;
    bool prepared = false;

    friend class IOUring<T>;
    IOUringSqe(std::shared_ptr<io_uring> ring, io_uring_sqe* sqe): ring(ring), sqe(sqe) {
    }
};

template<typename T>
class IOUringCqe {
    std::shared_ptr<io_uring> ring;
    io_uring_cqe* cqe;
    bool seen = false;
    IOUringUserData<T>* ud;

    friend class IOUring<T>;
    IOUringCqe(std::shared_ptr<io_uring> ring, io_uring_cqe* cqe): ring(ring), cqe(cqe) {
        ud = reinterpret_cast<IOUringUserData<T>*>(cqe->user_data);
    }

public:
    std::unique_ptr<T> getUserData() {
        return ud->getUserData();
    }

    Arg getArg(int pos) {
        return ud->args().at(0);
    }

    int res() {
        return cqe->res;
    }

    void markAsSeen() {
        if(!seen) {
            seen = true;
            io_uring_cqe_seen(ring.get(), cqe);
        }
    }

    ~IOUringCqe() {
        delete ud;
        if(!seen) {
            io_uring_cqe_seen(ring.get(), cqe);
        }
    }
};
