#include "CommandHistory.h"
#include <stdexcept>

CommandHistory::CommandHistory(std::shared_ptr<ILogger> logger) 
    : logger_(std::move(logger))
    , currentIndex_(-1)
{
}

void CommandHistory::executeCommand(std::shared_ptr<ICommand> command) {
    if (!command) {
        if (logger_) {
            logger_->warning("CommandHistory: Attempted to execute null command");
        }
        return;
    }
    
    try {
        if (currentIndex_ < static_cast<int>(history_.size()) - 1) {
            history_.erase(history_.begin() + currentIndex_ + 1, history_.end());
        }
        
        command->execute();
        
        history_.push_back(std::move(command));
        ++currentIndex_;
        
        if (logger_) {
            logger_->log("Command executed, history size: " + std::to_string(history_.size()));
        }
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->error("CommandHistory: Command execution failed - " + std::string(e.what()));
        }
        throw;
    }
}

void CommandHistory::undo() {
    if (!canUndo()) {
        if (logger_) {
            logger_->warning("CommandHistory: Nothing to undo");
        }
        return;
    }
    
    try {
        history_[currentIndex_]->undo();
        --currentIndex_;
        
        if (logger_) {
            logger_->log("Undo performed, position: " + std::to_string(currentIndex_));
        }
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->error("CommandHistory: Undo failed - " + std::string(e.what()));
        }
        throw;
    }
}

void CommandHistory::redo() {
    if (!canRedo()) {
        if (logger_) {
            logger_->warning("CommandHistory: Nothing to redo");
        }
        return;
    }
    
    try {
        ++currentIndex_;
        history_[currentIndex_]->redo();
        
        if (logger_) {
            logger_->log("Redo performed, position: " + std::to_string(currentIndex_));
        }
    } catch (const std::exception& e) {
        --currentIndex_;
        if (logger_) {
            logger_->error("CommandHistory: Redo failed - " + std::string(e.what()));
        }
        throw;
    }
}

bool CommandHistory::canUndo() const noexcept {
    return currentIndex_ >= 0 && !history_.empty();
}

bool CommandHistory::canRedo() const noexcept {
    return currentIndex_ < static_cast<int>(history_.size()) - 1;
}

void CommandHistory::clear() noexcept {
    history_.clear();
    currentIndex_ = -1;
    
    if (logger_) {
        logger_->log("CommandHistory: History cleared");
    }
}

std::string CommandHistory::getUndoDescription() const {
    if (!canUndo()) {
        return "";
    }
    return history_[currentIndex_]->getDescription();
}

std::string CommandHistory::getRedoDescription() const {
    if (!canRedo()) {
        return "";
    }
    return history_[currentIndex_ + 1]->getDescription();
}
