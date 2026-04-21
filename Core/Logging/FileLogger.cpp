#include "FileLogger.h"
#include <stdexcept>
#include <chrono>
#include <iomanip>

FileLogger::FileLogger(const std::string& filePath) {
    file_.open(filePath, std::ios::out | std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filePath);
    }
}

FileLogger::~FileLogger() {
    if (file_.is_open()) {
        file_.close();
    }
}

void FileLogger::log(const std::string& message) {
    writeLog("INFO", message);
}

void FileLogger::error(const std::string& message) {
    writeLog("ERROR", message);
}

void FileLogger::warning(const std::string& message) {
    writeLog("WARNING", message);
}

void FileLogger::writeLog(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!file_.is_open()) return;
    
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);

    file_ << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] "
          << "[" << level << "] " 
          << message << '\n';

    file_.flush();
}
