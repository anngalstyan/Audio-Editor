#pragma once

#include "ILogger.h"

class ConsoleLogger : public ILogger {
public:
    ConsoleLogger() = default;
    ~ConsoleLogger() override = default;
    
    void log(const std::string& message) override;
    void error(const std::string& message) override;
    void warning(const std::string& message) override;
};
