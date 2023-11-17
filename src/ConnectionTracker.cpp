#include "ConnectionTracker.hpp"

void ConnectionTrackerBase::publish(ConnectionEvent event) {
    for(auto& subscriber: subscribers) {
        subscriber->notify(event);
    }
}

void ConnectionTrackerBase::subscribe(std::shared_ptr<ConnectionSubscriber> subscriber) {
    std::lock_guard g(mutex);
    subscribers.push_back(subscriber);
}

void ConnectionTrackerBase::unsubscribe(std::shared_ptr<ConnectionSubscriber> subscriber) {
    std::lock_guard g(mutex);
    auto foundSubscriber = std::find(subscribers.begin(), subscribers.end(), subscriber);
    if(foundSubscriber != subscribers.end()) {
        subscribers.erase(foundSubscriber, foundSubscriber);
    }
}
