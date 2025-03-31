#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
using namespace std;

template<class T>
class TSQ
{
    private:
        queue<T> sharedQueue;
        mutex mut;
        condition_variable cv;
        bool canWrite = true;

        void writeImpl(const T& new_data) {
            unique_lock<mutex> mut(this->mut);
            this->cv.wait(mut, [this]() {return this->canWrite;});
            this->sharedQueue.push(new_data);
            mut.unlock();
            this->cv.notify_one();
        }

    public:
        TSQ() = default;
        T read()
        {
            unique_lock<mutex> mut(this->mut);
            this->cv.wait(mut, [this]() {return !this->sharedQueue.empty();});
            canWrite = false;
            T read_data = std::move(this->sharedQueue.front());
            this->sharedQueue.pop();
            mut.unlock();
            canWrite = true;
            this->cv.notify_one();
            return read_data;
        }
        T& peek()
        {
            unique_lock<mutex> mut(this->mut);
            this->cv.wait(mut, [this]() {return !this->sharedQueue.empty();});
            canWrite = false;
            T& read_data = this->sharedQueue.front();
            mut.unlock();
            canWrite = true;
            this->cv.notify_one();
            return read_data;
        }
        void write(const T& new_data)
        {
            writeImpl(new_data);
        }
        void write(T&& new_data)
        {
            writeImpl(new_data);
        }
        int getSize()
        {
            unique_lock<mutex> mut(this->mut);
            auto size = this->sharedQueue.size();
            mut.unlock();
            return size;
        }
        void clearQueue()
        {
            unique_lock<mutex> mut(this->mut);
            queue<T> emptyQueue; 
            this->sharedQueue.swap(emptyQueue);
        }
        queue<T> &getQueue()
        {
            return this->sharedQueue;
        }
        bool isEmpty()
        {
            unique_lock<mutex> mut(this->mut);
            auto empty = this->sharedQueue.empty();
            return empty;
        }
};