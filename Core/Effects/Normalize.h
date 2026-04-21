#pragma once

#include "IEffect.h"
#include "../Logging/ILogger.h"
#include "../Constants.h"
#include <memory>

class NormalizeEffect : public IEffect {
public:
    explicit NormalizeEffect(std::shared_ptr<ILogger> logger, 
                            float targetRMS = audio::normalize::kDefaultTargetRMS);
    ~NormalizeEffect() override = default;
    
    void apply(std::vector<float>& audioBuffer) override;
    void setParameter(const std::string& name, float value) override;
     std::string getName() const noexcept override { return "Normalize"; }

     std::shared_ptr<IEffect> clone() const override;
    
    void setTargetRMS(float rms) noexcept { targetRMS_ = rms; }
    void setTargetPeak(float peak) noexcept { targetPeak_ = peak; }
    
     float getTargetRMS() const noexcept { return targetRMS_; }
     float getTargetPeak() const noexcept { return targetPeak_; }

private:
     static float calculateRMS(const std::vector<float>& buffer);
     static float calculatePeak(const std::vector<float>& buffer);

    std::shared_ptr<ILogger> logger_;
    float targetRMS_;
    float targetPeak_;
};
