#pragma once

#include <mutex>
#include <shared_mutex>
#include <map>
#include <optional>

template<typename Key, typename Value>
class TSM
{
    private:
    std::map<Key, Value> map;
    mutable std::shared_mutex mut;
    public:
    void insert(const Key& key, const Value& value) 
    {
        std::unique_lock<std::shared_mutex> lock(this->mut);
        map[key] = value;
    }
    std::optional<Value> get(const Key& key) const 
    {
        std::shared_lock<std::shared_mutex> lock(this->mut);
        auto it = map.find(key);
        if (it != map.end()) 
        {
            return it->second;
        }
        return std::nullopt;
    }
};