#pragma once

#include "ICommand.h"
#include "EffectState.h"
#include "../Logging/ILogger.h"
#include <memory>

class EffectsPanel;

class EffectStateCommand : public ICommand {
public:
    EffectStateCommand(EffectsPanel* panel, 
                       EffectsPanelState oldState,
                       EffectsPanelState newState,
                       std::shared_ptr<ILogger> logger);

    void execute() override;
    void undo() override;
    void redo() override;
    
     std::string getDescription() const override {
        return "Effect Change";
    }

private:
    EffectsPanel* panel_;
    EffectsPanelState oldState_;
    EffectsPanelState newState_;
    std::shared_ptr<ILogger> logger_;
    bool executed_;
};
