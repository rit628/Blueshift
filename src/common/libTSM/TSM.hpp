#pragma once

#include <mutex>
#include <shared_mutex>
#include <map>
#include <optional>
#include <unordered_map>

template<typename Key, typename Value>
class TSM
{
    private:
        std::unordered_map<Key, Value> map;
        mutable std::shared_mutex mut;
    public:
        void insert(const Key& key, const Value& value) 
        {
            std::unique_lock<std::shared_mutex> lock(this->mut);
            map[key] = value;
        }

        void remove(const Key& key) {
            std::unique_lock<std::shared_mutex> lock(this->mut);
            map.erase(key);
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

        int getSize(){
            std::shared_lock<std::shared_mutex> lock(this->mut);
            return this->map.size();  

        }

        bool contains(const Key& key){
            std::shared_lock<std::shared_mutex> lock(this->mut); 
            return this->map.contains(key); 
        }

        // Should be safe since it copies by value? 
        std::unordered_map<Key, Value> getMap(){
            std::shared_lock<std::shared_mutex> lock(this->mut); 
            return this->map; 
        }

        void clear(){
            std::shared_lock<std::shared_mutex> lock(this->mut);
            this->map.clear();
        }
};