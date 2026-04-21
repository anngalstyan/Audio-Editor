#pragma once

#include "ILogger.h"
#include <fstream>
#include <mutex>
#include <string>

class FileLogger : public ILogger {
public:

    explicit FileLogger(const std::string& filePath);
    ~FileLogger() override;
    
    FileLogger(const FileLogger&) = delete;
    FileLogger& operator=(const FileLogger&) = delete;
    FileLogger(FileLogger&&) = delete;
    FileLogger& operator=(FileLogger&&) = delete;
    
    void log(const std::string& message) override;
    void error(const std::string& message) override;
    void warning(const std::string& message) override;
    
private:
    void writeLog(const std::string& level, const std::string& message);
    
    std::ofstream file_;
    std::mutex mutex_;
};
