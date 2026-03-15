#include "FileWatcher.hpp"
#include <stdexcept>
#ifdef __linux__
#include <sys/inotify.h>
#include <linux/limits.h>
#include <unistd.h>
#endif

void FileWatcher::watch(std::stop_token stoken) {
    #ifdef __linux__
    if (inotifyDescriptor < 0) return;
    constexpr size_t bufferSize = sizeof(inotify_event) + NAME_MAX + 1; 
    char eventBuffer[bufferSize];
    while (!stoken.stop_requested()) {
        size_t numBytes [[ maybe_unused ]] = read(inotifyDescriptor, eventBuffer, bufferSize);
        auto* event = reinterpret_cast<inotify_event *>(eventBuffer);
        if (event->mask != IN_CLOSE_WRITE) continue;
        if (descriptorMap.contains(event->wd)) {
            auto& filename = descriptorMap.at(event->wd);
            std::invoke(callbacks.at(filename));
        }
    }
    #endif
}

void FileWatcher::addWatch(const std::string& filename, std::function<void()> callback) {
    #ifdef __linux__
    if (inotifyDescriptor < 0) { // dont create a new inotify instance if one already exists
        inotifyDescriptor = inotify_init();
        if (inotifyDescriptor < 0) throw std::runtime_error("Unable to start inotify");
    }
    int watchDescriptor = inotify_add_watch(inotifyDescriptor, filename.c_str(), IN_CLOSE_WRITE);
    if (watchDescriptor < 0) throw std::runtime_error("Unable to add watch for file: " + filename);
    descriptorMap.insert(watchDescriptor, filename);
    callbacks.insert(filename, callback);
    #endif

    if (!watchThread.joinable()) { // start watcher thread if not already running
        watchThread = std::jthread(std::bind(&FileWatcher::watch, std::ref(*this), std::placeholders::_1));
    }
}

void FileWatcher::removeWatch(const std::string& filename) {
    if (callbacks.contains(filename)) {
        callbacks.insert(filename, [](){});
    }
}

FileWatcher::~FileWatcher() {
    watchThread.request_stop();
    #ifdef __linux__
    // unblock read() by generating IN_IGNORE events with rm_watch
    for (auto&& [watchDescriptor, _] : descriptorMap.getMap()) {
        inotify_rm_watch(inotifyDescriptor, watchDescriptor);
    }
    #endif
}