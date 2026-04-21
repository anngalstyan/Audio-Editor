#include "ApplyEffect.h"
#include "../Effects/Reverb.h"
#include "../Effects/Speed.h"

ApplyEffectCommand::ApplyEffectCommand(std::shared_ptr<AudioClip> clip,
                                       std::shared_ptr<IEffect> effect,
                                       std::shared_ptr<ILogger> logger)
    : clip_(std::move(clip))
    , logger_(std::move(logger))
    , executed_(false)
{
    if (effect) {
        effects_.push_back(std::move(effect));
    }
}

ApplyEffectCommand::ApplyEffectCommand(std::shared_ptr<AudioClip> clip,
                                       std::vector<std::shared_ptr<IEffect>> effects,
                                       std::shared_ptr<ILogger> logger)
    : clip_(std::move(clip))
    , effects_(std::move(effects))
    , logger_(std::move(logger))
    , executed_(false)
{
}

void ApplyEffectCommand::execute() {
    if (!clip_ || !clip_->isLoaded()) {
        if (logger_) {
            logger_->error("ApplyEffectCommand: No valid clip to apply effects");
        }
        return;
    }
    
    if (!executed_) {
        beforeState_ = clip_->getSamples();
        
        if (beforeState_.empty()) {
            if (logger_) {
                logger_->warning("ApplyEffectCommand: Empty sample buffer");
            }
            return;
        }
        
        std::vector<float> samples = beforeState_;
        
        for (const auto& effect : effects_) {
            if (!effect) continue;
            
            if (auto* reverb = dynamic_cast<Reverb*>(effect.get())) {
                reverb->reset();
            }
            
            effect->apply(samples);
        }
        
        clip_->setSamples(samples);
        afterState_ = samples;
        executed_ = true;
        
        if (logger_) {
            logger_->log("Effects applied and state saved for undo");
        }
    } else {
        clip_->setSamples(afterState_);
        
        if (logger_) {
            logger_->log("Effects re-applied (redo)");
        }
    }
}

void ApplyEffectCommand::undo() {
    if (!clip_) {
        if (logger_) {
            logger_->error("ApplyEffectCommand: No clip for undo");
        }
        return;
    }
    
    clip_->setSamples(beforeState_);
    
    if (logger_) {
        logger_->log("Effects undone - restored " + 
                     std::to_string(beforeState_.size()) + " samples");
    }
}
