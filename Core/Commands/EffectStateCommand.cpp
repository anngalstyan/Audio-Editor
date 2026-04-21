#include "EffectStateCommand.h"
#include "../../GUI/EffectsPanel.h"

EffectStateCommand::EffectStateCommand(EffectsPanel* panel,
                                       EffectsPanelState oldState,
                                       EffectsPanelState newState,
                                       std::shared_ptr<ILogger> logger)
    : panel_(panel)
    , oldState_(std::move(oldState))
    , newState_(std::move(newState))
    , logger_(std::move(logger))
    , executed_(false)
{
}

void EffectStateCommand::execute() {
    if (!panel_) return;
    
    if (!executed_) {
        executed_ = true;
        if (logger_) {
            logger_->log("EffectStateCommand: Initial execution recorded");
        }
    } else {
        panel_->restoreState(newState_);
        if (logger_) {
            logger_->log("EffectStateCommand: Re-executed (redo)");
        }
    }
}

void EffectStateCommand::undo() {
    if (!panel_) return;
    
    if (logger_) {
        logger_->log("EffectStateCommand: Undoing effect state change");
    }
    
    panel_->restoreState(oldState_);
}

void EffectStateCommand::redo() {
    if (!panel_) return;
    
    if (logger_) {
        logger_->log("EffectStateCommand: Redoing effect state change");
    }
    
    panel_->restoreState(newState_);
}
