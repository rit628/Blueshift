#pragma once
#include <filesystem>
#include <functional>

namespace FileNotify {
    void addWatch(const std::filesystem::path& file, const std::function<void()>& callback);
    void enableWatch(const std::filesystem::path& file, const std::function<void()>& callback);
    void disableWatch(const std::filesystem::path& file);
    void terminate();
}