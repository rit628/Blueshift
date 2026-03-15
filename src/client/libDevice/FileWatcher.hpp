#pragma once
#include "TSM.hpp"
#include <functional>
#include <stop_token>
#include <string>
#include <thread>
#ifdef __linux__
#include <sys/inotify.h>
#endif

class FileWatcher {
    public:
        void addWatch(const std::string& filename, std::function<void()> callback);
        void removeWatch(const std::string& filename);
        ~FileWatcher();
    
    private:
        void watch(std::stop_token stoken);

        TSM<std::string, std::function<void()>> callbacks;
        std::jthread watchThread;
        #ifdef __linux__
        int inotifyDescriptor = -1;
        TSM<int, std::string> descriptorMap;
        #endif
};