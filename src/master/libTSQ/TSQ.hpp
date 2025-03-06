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
        bool isReading = true;
        bool canWrite = true;
    public:
        TSQ() = default;
        T read()
        {
            unique_lock<mutex> mut(this->mut);
            this->cv.wait(mut, [this]() {return !this->sharedQueue.empty();});
            canWrite = false;
            T read_data = this->sharedQueue.front();
            this->sharedQueue.pop();
            mut.unlock();
            canWrite = true;
            this->cv.notify_one();
            return read_data;
        }
        T peek()
        {
            unique_lock<mutex> mut(this->mut);
            this->cv.wait(mut, [this]() {return !this->sharedQueue.empty();});
            canWrite = false;
            T read_data = this->sharedQueue.front();
            mut.unlock();
            canWrite = true;
            this->cv.notify_one();
            return read_data;
        }
        void write(T new_data)
        {
            unique_lock<mutex> mut(this->mut);
            this->cv.wait(mut, [this]() {return this->canWrite;});
            this->sharedQueue.push(new_data);
            mut.unlock();
            this->cv.notify_one();
        }
        int getSize()
        {
            return this->sharedQueue.size();
        }
        void clearQueue()
        {
            queue<T> emptyQueue; 
            this->sharedQueue.swap(emptyQueue);
        }
        queue<T> &getQueue()
        {
            return this->sharedQueue;
        }
        bool isEmpty()
        {
            if(this->sharedQueue.empty()){return true;}
            else{return false;}
        }
};