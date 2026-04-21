#include "AudioClip.h"
#include "Adapters/Mp3.h"
#include "Effects/Normalize.h"
#include <algorithm>
#include <cmath>
#include <numeric>

AudioClip::AudioClip(const std::string& filePath, std::shared_ptr<ILogger> logger) 
    : filePath_(filePath)
    , logger_(std::move(logger))
{
    audioFile_ = std::make_unique<Mp3Adapter>(logger_);
}

bool AudioClip::load() {
    if (!audioFile_->load(filePath_)) {
        if (logger_) {
            logger_->error("Failed to load file: " + filePath_);
        }
        return false;
    }

    samples_ = std::vector<float>(audioFile_->getSamples());
    isLoaded_ = true;
    
    if (logger_) {
        logger_->log("Loaded " + std::to_string(samples_.size()) + 
                     " samples from " + filePath_);
    }
    
    return true;
}

bool AudioClip::save(const std::string& outputPath) {
    if (!isLoaded_ || samples_.empty()) {
        if (logger_) {
            logger_->error("Cannot save: no samples loaded");
        }
        return false;
    }

    if (logger_) {
        const auto [minIt, maxIt] = std::minmax_element(samples_.begin(), samples_.end());
        const double sumSquares = std::accumulate(samples_.begin(), samples_.end(), 0.0,
            [](double acc, float s) { return acc + s * s; });
        const double rms = std::sqrt(sumSquares / samples_.size());
        
        logger_->log("Saving samples - Min: " + std::to_string(*minIt) +
                     ", Max: " + std::to_string(*maxIt) +
                     ", RMS: " + std::to_string(rms));
    }

    return audioFile_->save(outputPath, samples_);
}

void AudioClip::addEffect(std::shared_ptr<IEffect> effect) {
    if (effect) {
        effects_.push_back(std::move(effect));
    }
}

void AudioClip::applyEffects() {
    if (!isLoaded_ || samples_.empty()) return;

    const double sumSquaresBefore = std::accumulate(samples_.begin(), samples_.end(), 0.0,
        [](double acc, float s) { return acc + s * s; });
    const double rmsBefore = std::sqrt(sumSquaresBefore / samples_.size());
    
    if (logger_) {
        logger_->log("Before effects - RMS: " + std::to_string(rmsBefore));
    }

    for (auto& effect : effects_) {
        if (effect) {
            effect->apply(samples_);
        }
    }

    auto normalizer = std::make_shared<NormalizeEffect>(logger_, static_cast<float>(rmsBefore));
    normalizer->apply(samples_);

    if (logger_) {
        const double sumSquaresAfter = std::accumulate(samples_.begin(), samples_.end(), 0.0,
            [](double acc, float s) { return acc + s * s; });
        const double rmsAfter = std::sqrt(sumSquaresAfter / samples_.size());
        logger_->log("After effects - RMS: " + std::to_string(rmsAfter));
    }
}

void AudioClip::setSamples(std::vector<float> samples) {
    samples_ = std::move(samples);
    
    if (logger_) {
        logger_->log("Samples updated: " + std::to_string(samples_.size()) + " samples");
    }
}
