#include "Speed.h"
#include <algorithm>
#include <cmath>

SpeedChangeEffect::SpeedChangeEffect(float speedFactor, std::shared_ptr<ILogger> logger)
    : speedFactor_(std::clamp(speedFactor, 
                              audio::speed::kMinSpeedFactor, 
                              audio::speed::kMaxSpeedFactor))
    , logger_(std::move(logger))
{
}

std::shared_ptr<IEffect> SpeedChangeEffect::clone() const {
    return std::make_shared<SpeedChangeEffect>(speedFactor_, logger_);
}

void SpeedChangeEffect::setSpeedFactor(float speedFactor) {
    speedFactor_ = std::clamp(speedFactor, 
                              audio::speed::kMinSpeedFactor, 
                              audio::speed::kMaxSpeedFactor);
}

void SpeedChangeEffect::setParameter(const std::string& name, float value) {
    if (name == "speed") {
        setSpeedFactor(value);
    }
}

static float hermite(float y0, float y1, float y2, float y3, float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    
    // Hermite basis coefficients
    const float c0 = y1;                                        // Position at t=0
    const float c1 = 0.5f * (y2 - y0);                         // Tangent at t=0
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;   // Curvature
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);      // Higher order term
    
    return c0 + c1 * t + c2 * t2 + c3 * t3;
}

void SpeedChangeEffect::apply(std::vector<float>& audioBuffer) {
    if (std::abs(speedFactor_ - 1.0f) < 0.001f || audioBuffer.empty()) {
        return;
    }

    constexpr int channels = 2;
    
    if (audioBuffer.size() % channels != 0) {
        if (logger_) {
            logger_->warning("Odd buffer size (" + std::to_string(audioBuffer.size()) + 
                            "), padding with silence");
        }
        audioBuffer.push_back(0.0f);
    }
    
    const size_t inputFrames = audioBuffer.size() / channels;
    const size_t outputFrames = static_cast<size_t>(static_cast<double>(inputFrames) / speedFactor_);
    
    if (outputFrames == 0) {
        if (logger_) {
            logger_->warning("Speed change would produce 0 frames, skipping");
        }
        return;
    }
    
    std::vector<float> output(outputFrames * channels, 0.0f);

    for (size_t i = 0; i < outputFrames; ++i) {
        const double srcPos = static_cast<double>(i) * speedFactor_;
        const size_t srcIdx = static_cast<size_t>(srcPos);
        const float t = static_cast<float>(srcPos - srcIdx);
        
        for (int ch = 0; ch < channels; ++ch) {
            auto getSample = [&](size_t idx) -> float {
                if (idx >= inputFrames) idx = inputFrames - 1;
                return audioBuffer[idx * channels + ch];
            };
            
            // Get 4 samples for Hermite interpolation
            const size_t idx0 = (srcIdx > 0) ? srcIdx - 1 : 0;
            const size_t idx1 = srcIdx;
            const size_t idx2 = srcIdx + 1;
            const size_t idx3 = srcIdx + 2;
            
            const float y0 = getSample(idx0);
            const float y1 = getSample(idx1);
            const float y2 = getSample(idx2);
            const float y3 = getSample(idx3);
            
            float sample = hermite(y0, y1, y2, y3, t);
            output[i * channels + ch] = std::clamp(sample, -1.0f, 1.0f);
        }
    }

    audioBuffer = std::move(output);
}