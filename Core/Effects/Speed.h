#pragma once

#include "IEffect.h"
#include "../Logging/ILogger.h"
#include "../Constants.h"
#include <memory>

class SpeedChangeEffect : public IEffect {
public:
    SpeedChangeEffect(float speedFactor, std::shared_ptr<ILogger> logger);
    ~SpeedChangeEffect() override = default;
    
    void apply(std::vector<float>& audioBuffer) override;
    void setParameter(const std::string& name, float value) override;
     std::string getName() const noexcept override { return "Speed"; }

     std::shared_ptr<IEffect> clone() const override;
    
    void setSpeedFactor(float speedFactor);
     float getSpeedFactor() const noexcept { return speedFactor_; }

private:
    float speedFactor_;
    std::shared_ptr<ILogger> logger_;
};
