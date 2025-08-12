#pragma once

#include <optional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <stop_token>
using namespace std;

template<class T>
class TSQ {
private:
    std::queue<T> sharedQueue;
    std::mutex mut;
    std::condition_variable_any cv;

public:
    TSQ() = default;

    // Read and remove the front item from the queue (blocking if empty)
    T read() {
        std::unique_lock<std::mutex> lock(mut);
        cv.wait(lock, [this]() { return !sharedQueue.empty(); });

        T data = sharedQueue.front();
        sharedQueue.pop();
        return data;
    }

    std::optional<T> read(std::stop_token& stoken) {
        std::unique_lock<std::mutex> lock(mut);
        if (!cv.wait(lock, stoken, [this]() { return !sharedQueue.empty(); })) {
            return std::nullopt;
        }

        T data = sharedQueue.front();
        sharedQueue.pop();
        return data;
    }
    
    std::optional<T> pop() {
        std::lock_guard<std::mutex> lock(mut);
        if (sharedQueue.empty()) {
            return std::nullopt;
        }
        T data = sharedQueue.front();
        sharedQueue.pop();
        return data;
    }

    // Peek at the front item without removing it (blocking if empty)
    T peek() {
        std::unique_lock<std::mutex> lock(mut);
        cv.wait(lock, [this]() { return !sharedQueue.empty(); });

        return sharedQueue.front();
    }

    std::optional<T> peek(std::stop_token& stoken) {
        std::unique_lock<std::mutex> lock(mut);
        if (!cv.wait(lock, stoken, [this]() { return !sharedQueue.empty(); })) {
            return std::nullopt;
        }

        return sharedQueue.front();
    }

    // Write new data to the queue and notify one waiting reader
    void write(const T& new_data) {
        {
            std::lock_guard<std::mutex> lock(mut);
            sharedQueue.push(new_data);
        }
        cv.notify_one();  // Wake up one waiting thread (read/peek)
    }

    // Returns current size of the queue (thread-safe)
    int getSize() {
        std::lock_guard<std::mutex> lock(mut);
        return static_cast<int>(sharedQueue.size());
    }

    // Clears the queue (thread-safe)
    void clearQueue() {
        std::lock_guard<std::mutex> lock(mut);
        std::queue<T> emptyQueue;
        std::swap(sharedQueue, emptyQueue);
    }

    // Returns true if the queue is empty (thread-safe)
    bool isEmpty() {
        std::lock_guard<std::mutex> lock(mut);
        return sharedQueue.empty();
    }

    // Deleted getQueue() to enforce encapsulation
};