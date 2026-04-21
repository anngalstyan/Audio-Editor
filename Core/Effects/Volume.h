#pragma once

#include "IEffect.h"
#include "../Logging/ILogger.h"
#include "../Constants.h"
#include <memory>

class VolumeEffect : public IEffect {
public:
    VolumeEffect(float gain, std::shared_ptr<ILogger> logger);
    ~VolumeEffect() override = default;
    
    void apply(std::vector<float>& audioBuffer) override;
    void setParameter(const std::string& name, float value) override;
     std::string getName() const noexcept override { return "Volume"; }

     std::shared_ptr<IEffect> clone() const override;
    
    void setGain(float gain);
     float getGain() const noexcept { return gain_; }

private:
    float gain_;
    std::shared_ptr<ILogger> logger_;
};
