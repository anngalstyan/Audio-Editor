#include "Normalize.h"
#include <cmath>
#include <algorithm>
#include <numeric>

NormalizeEffect::NormalizeEffect(std::shared_ptr<ILogger> logger, float targetRMS)
    : logger_(std::move(logger))
    , targetRMS_(targetRMS)
    , targetPeak_(audio::normalize::kDefaultTargetPeak)
{
}

std::shared_ptr<IEffect> NormalizeEffect::clone() const {
    auto copy = std::make_shared<NormalizeEffect>(logger_, targetRMS_);
    copy->setTargetPeak(targetPeak_);
    return copy;
}

void NormalizeEffect::setParameter(const std::string& name, float value) {
    if (name == "targetRMS") {
        setTargetRMS(value);
    } else if (name == "targetPeak") {
        setTargetPeak(value);
    }
}

float NormalizeEffect::calculateRMS(const std::vector<float>& buffer) {
    if (buffer.empty()) return 0.0f;
    
    const double sumSquares = std::accumulate(buffer.begin(), buffer.end(), 0.0,
        [](double acc, float sample) { return acc + sample * sample; });
    
    return static_cast<float>(std::sqrt(sumSquares / buffer.size()));
}

float NormalizeEffect::calculatePeak(const std::vector<float>& buffer) {
    if (buffer.empty()) return 0.0f;
    
    float peak = 0.0f;
    for (const float sample : buffer) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

void NormalizeEffect::apply(std::vector<float>& buffer) {
    if (buffer.empty()) return;
    
    const float currentRMS = calculateRMS(buffer);
    const float currentPeak = calculatePeak(buffer);
    
    if (logger_) {
        logger_->log("Before normalize - RMS: " + std::to_string(currentRMS) +
                     ", Peak: " + std::to_string(currentPeak));
    }
    
    // Calculate and apply RMS-based gain
    const float rmsGain = (currentRMS > audio::normalize::kMinRMSThreshold) 
                         ? (targetRMS_ / currentRMS) 
                         : 1.0f;
    
    for (float& sample : buffer) {
        sample *= rmsGain;
    }
    
    // Peak limiting to prevent clipping
    const float newPeak = calculatePeak(buffer);
    if (newPeak > targetPeak_) {
        const float peakGain = targetPeak_ / newPeak;
        for (float& sample : buffer) {
            sample *= peakGain;
        }
        
        if (logger_) {
            logger_->log("Clipping prevented - applied limiter (gain: " +
                         std::to_string(peakGain) + "x)");
        }
    }
    
    if (logger_) {
        const float finalRMS = calculateRMS(buffer);
        const float finalPeak = calculatePeak(buffer);
        logger_->log("After normalize - RMS: " + std::to_string(finalRMS) +
                     ", Peak: " + std::to_string(finalPeak) +
                     ", Gain applied: " + std::to_string(rmsGain));
    }
}