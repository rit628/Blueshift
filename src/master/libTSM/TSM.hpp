#pragma once

#include <mutex>
#include <shared_mutex>
#include <map>
#include <optional>
using namespace std;

template<typename Key, typename Value>
class TSM
{
    private:
    map<Key, Value> map;
    mutable shared_mutex mut;
    public:
    void insert(const Key& key, const Value& value) 
    {
        unique_lock<shared_mutex> lock(this->mut);
        map[key] = value;
    }
    optional<Value> get(const Key& key) const 
    {
        shared_lock<shared_mutex> lock(this->mut);
        auto it = map.find(key);
        if (it != map.end()) 
        {
            return it->second;
        }
        return nullopt;
    }
};