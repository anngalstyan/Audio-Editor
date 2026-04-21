#include "Reverb.h"
#include "../Constants.h"
#include <cmath>
#include <algorithm>

Reverb::Reverb(std::shared_ptr<ILogger> logger)
    : intensity_(0.5f)
    , logger_(std::move(logger))
    , initialized_(false)
{
    combIndices_.fill(0);
    combFilterStore_.fill(0.0f);
    allpassIndices_.fill(0);
}

std::shared_ptr<IEffect> Reverb::clone() const {
    auto copy = std::make_shared<Reverb>(logger_);
    copy->setIntensity(intensity_);
    // Note: We don't copy buffer state - clone starts fresh
    // This is intentional for thread-safe processing
    return copy;
}

void Reverb::setIntensity(float intensity) {
    intensity_ = std::clamp(intensity, 0.0f, 1.0f);
}

void Reverb::setParameter(const std::string& name, float value) {
    if (name == "intensity") {
        setIntensity(value);
    }
}

void Reverb::reset() {
    for (auto& buf : combBuffersL_) std::fill(buf.begin(), buf.end(), 0.0f);
    for (auto& buf : combBuffersR_) std::fill(buf.begin(), buf.end(), 0.0f);
    for (auto& buf : allpassBuffersL_) std::fill(buf.begin(), buf.end(), 0.0f);
    for (auto& buf : allpassBuffersR_) std::fill(buf.begin(), buf.end(), 0.0f);
    combIndices_.fill(0);
    combFilterStore_.fill(0.0f);
    allpassIndices_.fill(0);
}

void Reverb::initBuffers(int sampleRate) {
    using namespace audio::reverb;
    
    float scale = static_cast<float>(sampleRate) / audio::kDefaultSampleRate;
    
    for (int i = 0; i < kNumCombFilters; ++i) {
        int size = static_cast<int>(kCombDelays[i] * scale);
        combBuffersL_[i].resize(size, 0.0f);
        combBuffersR_[i].resize(size, 0.0f);
    }
    
    for (int i = 0; i < kNumAllpassFilters; ++i) {
        int size = static_cast<int>(kAllpassDelays[i] * scale);
        allpassBuffersL_[i].resize(size, 0.0f);
        allpassBuffersR_[i].resize(size, 0.0f);
    }
    
    initialized_ = true;
}

void Reverb::apply(std::vector<float>& audioBuffer) {
    using namespace audio::reverb;
    
    if (audioBuffer.empty() || intensity_ < 0.001f) {
        return;
    }
    
    if (!initialized_) {
        initBuffers(audio::kDefaultSampleRate);
    }
    
    const float t = std::sqrt(intensity_);
    const float feedback = kMinFeedback + t * (kMaxFeedback - kMinFeedback);
    const float damping = kMaxDamping - t * (kMaxDamping - kMinDamping);
    const float wetMix = kMinWetMix + t * (kMaxWetMix - kMinWetMix);
    const float dryMix = 1.0f - (wetMix * 0.5f);
    
    if (logger_) {
        logger_->log("Reverb: intensity=" + std::to_string(intensity_) +
                    " t=" + std::to_string(t) +
                    " wet=" + std::to_string(wetMix));
    }
    
    const size_t numSamples = audioBuffer.size() / 2;
    
    for (size_t i = 0; i < numSamples; ++i) {
        float inputL = audioBuffer[i * 2];
        float inputR = audioBuffer[i * 2 + 1];
        float input = (inputL + inputR) * 0.5f;
        
        float combOutL = 0.0f;
        float combOutR = 0.0f;
        
        for (int c = 0; c < kNumCombFilters; ++c) {
            auto& bufL = combBuffersL_[c];
            auto& bufR = combBuffersR_[c];
            size_t& idx = combIndices_[c];
            float& filterStore = combFilterStore_[c];
            
            float delayedL = bufL[idx];
            float delayedR = bufR[idx];
            
            filterStore = delayedL * (1.0f - damping) + filterStore * damping;
            
            bufL[idx] = input + filterStore * feedback;
            bufR[idx] = input + delayedR * (1.0f - damping) * feedback;
            
            combOutL += delayedL;
            combOutR += delayedR;
            
            idx = (idx + 1) % bufL.size();
        }
        
        combOutL /= kNumCombFilters;
        combOutR /= kNumCombFilters;
        
        float allpassOutL = combOutL;
        float allpassOutR = combOutR;
        
        for (int a = 0; a < kNumAllpassFilters; ++a) {
            auto& bufL = allpassBuffersL_[a];
            auto& bufR = allpassBuffersR_[a];
            size_t& idx = allpassIndices_[a];
            
            float delayedL = bufL[idx];
            float delayedR = bufR[idx];
            
            float tempL = -kAllpassGain * allpassOutL + delayedL;
            float tempR = -kAllpassGain * allpassOutR + delayedR;
            
            bufL[idx] = allpassOutL + kAllpassGain * delayedL;
            bufR[idx] = allpassOutR + kAllpassGain * delayedR;
            
            allpassOutL = tempL;
            allpassOutR = tempR;
            
            idx = (idx + 1) % bufL.size();
        }
        
        float outL = inputL * dryMix + allpassOutL * wetMix;
        float outR = inputR * dryMix + allpassOutR * wetMix;
        
        audioBuffer[i * 2] = std::clamp(outL, -1.0f, 1.0f);
        audioBuffer[i * 2 + 1] = std::clamp(outR, -1.0f, 1.0f);
    }
}