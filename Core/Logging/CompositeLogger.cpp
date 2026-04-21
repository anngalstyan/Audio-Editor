#include "CompositeLogger.h"
#include <algorithm>

void CompositeLogger::addLogger(std::shared_ptr<ILogger> logger) {
    if (logger) {
        loggers_.push_back(std::move(logger));
    }
}

void CompositeLogger::removeLogger(const std::shared_ptr<ILogger>& logger) {
    loggers_.erase(
        std::remove(loggers_.begin(), loggers_.end(), logger),
        loggers_.end()
    );
}

void CompositeLogger::clearLoggers() noexcept {
    loggers_.clear();
}

void CompositeLogger::log(const std::string& message) {
    for (const auto& logger : loggers_) {
        logger->log(message);
    }
}

void CompositeLogger::error(const std::string& message) {
    for (const auto& logger : loggers_) {
        logger->error(message);
    }
}

void CompositeLogger::warning(const std::string& message) {
    for (const auto& logger : loggers_) {
        logger->warning(message);
    }
}
