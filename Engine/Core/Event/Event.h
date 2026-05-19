#pragma once
#include <string>
#include <unordered_map>
#include <any>
#include <optional>

namespace fun {

class Event {
public:
    explicit Event(const std::string& type) : m_type(type) {}

    const std::string& GetType() const { return m_type; }

    template<typename T>
    void Set(const std::string& key, T value) {
        m_data[key] = std::move(value);
    }

    template<typename T>
    std::optional<T> Get(const std::string& key) const {
        auto it = m_data.find(key);
        if (it != m_data.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    bool Has(const std::string& key) const {
        return m_data.find(key) != m_data.end();
    }

private:
    std::string m_type;
    std::unordered_map<std::string, std::any> m_data;
};

} // namespace fun
