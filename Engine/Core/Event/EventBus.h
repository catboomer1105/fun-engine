#pragma once
#include "Event.h"
#include <functional>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace fun {

class EventBus {
public:
    using Callback = std::function<void(const Event&)>;
    using SubscriptionId = uint64_t;

    SubscriptionId Subscribe(const std::string& eventType, Callback callback) {
        SubscriptionId id = m_nextId++;
        m_subscribers[eventType].push_back({id, std::move(callback)});
        return id;
    }

    void Unsubscribe(SubscriptionId id) {
        for (auto& [type, list] : m_subscribers) {
            list.erase(
                std::remove_if(list.begin(), list.end(),
                    [id](const Subscriber& s) { return s.id == id; }),
                list.end());
        }
    }

    void Emit(const Event& event) {
        auto it = m_subscribers.find(event.GetType());
        if (it != m_subscribers.end()) {
            // 复制列表，允许回调中修改订阅
            auto copy = it->second;
            for (auto& subscriber : copy) {
                subscriber.callback(event);
            }
        }
    }

private:
    struct Subscriber {
        SubscriptionId id;
        Callback callback;
    };

    std::unordered_map<std::string, std::vector<Subscriber>> m_subscribers;
    SubscriptionId m_nextId = 1;
};

} // namespace fun
