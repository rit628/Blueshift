#include "FileNotify.hpp"
#include "TSM.hpp"
#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <thread>
#ifdef __linux__
#include <sys/inotify.h>
#include <linux/limits.h>
#include <unistd.h>
#endif
#ifdef __WIN32__
#include <codecvt>
#include <locale>
#include <windows.h>
#endif

namespace FileNotify {
    namespace fs = std::filesystem;

    namespace {
        TSM<fs::path, std::function<void()>> callbacks;
        std::jthread watchThread;
        #ifdef __linux__
        int inotifyDescriptor = -1;
        TSM<int, fs::path> descriptorMap;
        #endif
        #ifdef __WIN32__
        HANDLE watchDirectory = INVALID_HANDLE_VALUE;
        fs::path directoryPath = fs::current_path();
        #endif
        
        void watch(std::stop_token stoken) {
            #ifdef __linux__
            constexpr size_t bufferSize = sizeof(inotify_event) + NAME_MAX + 1; 
            char eventBuffer[bufferSize];
            while (!stoken.stop_requested()) {
                size_t numBytes [[ maybe_unused ]] = read(inotifyDescriptor, eventBuffer, bufferSize);
                auto* event = reinterpret_cast<inotify_event *>(eventBuffer);
                if (event->mask != IN_CLOSE_WRITE) continue;
                if (descriptorMap.contains(event->wd)) {
                    auto file = descriptorMap.at(event->wd);
                    std::invoke(callbacks.at(file));
                }
            }
            #endif

            #ifdef __WIN32__
            constexpr size_t bufferSize = sizeof(FILE_NOTIFY_INFORMATION) + MAX_PATH + 1;
            BYTE eventBuffer[bufferSize];
            while (!stoken.stop_requested()) {
                DWORD numBytes = 0;
                BOOL success = ReadDirectoryChangesW(
                    watchDirectory,
                    eventBuffer,
                    bufferSize,
                    FALSE,
                    FILE_NOTIFY_CHANGE_LAST_WRITE,
                    &numBytes,
                    NULL,
                    NULL
                );
                if (!success) continue;

                auto* event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(eventBuffer);
                if (event->Action != FILE_ACTION_MODIFIED) continue;
                std::wstring wname(event->FileName, event->FileNameLength / sizeof(WCHAR));
                std::string filename = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wname);
                auto file = (directoryPath / fs::path(filename)).make_preferred();
                if (callbacks.contains(file)) {
                    std::invoke(callbacks.at(file));
                }
            }
            #endif
        }
    }

    void addWatch(const fs::path& file, const std::function<void()>& callback) {
        #ifdef __linux__
        if (inotifyDescriptor < 0) { // dont create a new inotify instance if one already exists
            inotifyDescriptor = inotify_init();
            if (inotifyDescriptor < 0) throw std::runtime_error("Unable to start inotify");
        }
        int watchDescriptor = inotify_add_watch(inotifyDescriptor, file.c_str(), IN_CLOSE_WRITE);
        if (watchDescriptor < 0) throw std::runtime_error("Unable to add watch for file: " + file.string());
        descriptorMap.insert(watchDescriptor, file);
        #endif

        #ifdef __WIN32__
        auto parentDirectory = (file.has_parent_path()) ? file.parent_path() : fs::current_path();
        if (watchDirectory == INVALID_HANDLE_VALUE || parentDirectory < directoryPath) {
            if (watchDirectory != INVALID_HANDLE_VALUE) { // cancel existing watch if one already exists
                CancelIoEx(watchDirectory, NULL);
                CloseHandle(watchDirectory);
            }
            directoryPath = parentDirectory;
            watchDirectory = CreateFile(
                parentDirectory.string().c_str(),
                FILE_LIST_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,
                NULL
            );
            if (watchDirectory == INVALID_HANDLE_VALUE) throw std::runtime_error("Unable to watch directory");
        }
        #endif

        callbacks.insert(file, callback);
        if (!watchThread.joinable()) { // start watcher thread if not already running
            watchThread = std::jthread(std::bind(&watch, std::placeholders::_1));
            std::atexit(terminate);
        }
    }

    void enableWatch(const fs::path& file, const std::function<void()>& callback) {
        if (callbacks.contains(file)) {
            callbacks.insert(file,callback);
        }
    }

    void disableWatch(const fs::path& file) {
        if (callbacks.contains(file)) {
            callbacks.insert(file, [](){});
        }
    }

    void terminate() {
        watchThread.request_stop();
        #ifdef __linux__
        // unblock read() by generating IN_IGNORE events with rm_watch
        for (auto&& [watchDescriptor, _] : descriptorMap.getMap()) {
            inotify_rm_watch(inotifyDescriptor, watchDescriptor);
        }
        close(inotifyDescriptor);
        descriptorMap.clear();
        #endif
        #ifdef __WIN32__
        CancelIoEx(watchDirectory, NULL);
        CloseHandle(watchDirectory);
        #endif
        callbacks.clear();
    }

}