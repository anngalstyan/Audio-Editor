#pragma once

#include "ILogger.h"
#include <vector>
#include <memory>

class CompositeLogger : public ILogger {
public:
    CompositeLogger() = default;
    ~CompositeLogger() override = default;
    void addLogger(std::shared_ptr<ILogger> logger);
    void removeLogger(const std::shared_ptr<ILogger>& logger);
    void clearLoggers() noexcept;
     size_t size() const noexcept { return loggers_.size(); }
    
    void log(const std::string& message) override;
    void error(const std::string& message) override;
    void warning(const std::string& message) override;
    
private:
    std::vector<std::shared_ptr<ILogger>> loggers_;
};
