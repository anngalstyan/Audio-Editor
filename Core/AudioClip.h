#pragma once

#include <memory>
#include <vector>
#include <string>
#include "Adapters/AudioFileAdapter.h"
#include "Effects/IEffect.h"
#include "Logging/ILogger.h"

class AudioClip {
public:
    explicit AudioClip(const std::string& filePath, std::shared_ptr<ILogger> logger);
    ~AudioClip() = default;

    AudioClip(const AudioClip&) = delete;
    AudioClip& operator=(const AudioClip&) = delete;

    AudioClip(AudioClip&&) = default;
    AudioClip& operator=(AudioClip&&) = default;

     bool load();

     bool save(const std::string& outputPath);

    void addEffect(std::shared_ptr<IEffect> effect);

    void applyEffects();

    void clearEffects() noexcept { effects_.clear(); }
    
     const std::vector<float>& getSamples() const noexcept { 
        return samples_; 
    }
    
     std::vector<float>& getSamplesRef() noexcept { 
        return samples_; 
    }
    
    void setSamples(std::vector<float> samples);
    
     bool isLoaded() const noexcept { return isLoaded_; }
     const std::string& getFilePath() const noexcept { return filePath_; }
    
     int getSampleRate() const noexcept { 
        return audioFile_ ? audioFile_->getSampleRate() : 44100; 
    }
    
     int getChannels() const noexcept { 
        return audioFile_ ? audioFile_->getChannels() : 2; 
    }
    
     float getDuration() const noexcept {
        return audioFile_ ? audioFile_->getDuration() : 0.0f;
    }

private:
    std::string filePath_;
    std::unique_ptr<AudioFileAdapter> audioFile_;
    std::vector<float> samples_;
    std::vector<std::shared_ptr<IEffect>> effects_;
    bool isLoaded_ = false;
    std::shared_ptr<ILogger> logger_;
};