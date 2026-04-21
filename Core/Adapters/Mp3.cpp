#include "Mp3.h"
#include <mpg123.h>
#include <lame/lame.h>
#include <cmath>
#include <algorithm>

Mp3Adapter::Mp3Adapter(std::shared_ptr<ILogger> logger) 
    : logger_(std::move(logger))
{
}

bool Mp3Adapter::load(const std::string& filePath) {
    if (logger_) {
        logger_->log("Loading MP3 file from: " + filePath);
    }
    
    mpg123_handle* mh = mpg123_new(nullptr, nullptr);
    if (!mh) {
        if (logger_) {
            logger_->error("Failed to create mpg123 handle");
        }
        return false;
    }
    
    if (mpg123_open(mh, filePath.c_str()) != MPG123_OK) {
        if (logger_) {
            logger_->error("Failed to open MP3 file: " + filePath);
        }
        mpg123_delete(mh);
        return false;
    }
    
    long rate = 0;
    int channels = 0;
    int encoding = 0;
    if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
        if (logger_) {
            logger_->error("Failed to get MP3 format");
        }
        mpg123_close(mh);
        mpg123_delete(mh);
        return false;
    }
    
    sampleRate_ = static_cast<int>(rate);
    channels_ = channels;
    
    const off_t length = mpg123_length(mh);
    if (length == MPG123_ERR) {
        if (logger_) {
            logger_->error("Failed to get MP3 length");
        }
        mpg123_close(mh);
        mpg123_delete(mh);
        return false;
    }
    
    duration_ = static_cast<float>(length) / static_cast<float>(sampleRate_);
    
    samples_.clear();
    samples_.reserve(static_cast<size_t>(length * channels_));
    
    std::vector<unsigned char> buffer(audio::kMp3ReadBufferSize);
    size_t done = 0;
    
    while (mpg123_read(mh, buffer.data(), buffer.size(), &done) == MPG123_OK) {
        for (size_t i = 0; i + 1 < done; i += 2) {
            const auto sample = static_cast<short>((buffer[i + 1] << 8) | buffer[i]);
            samples_.push_back(static_cast<float>(sample) / audio::kSampleNormalizationFactor);
        }
    }
    
    mpg123_close(mh);
    mpg123_delete(mh);
    
    if (logger_) {
        logger_->log("Loaded MP3: " + filePath + 
                     " (" + std::to_string(samples_.size()) + " samples, " + 
                     std::to_string(duration_) + "s, " + 
                     std::to_string(channels_) + " channels)");
    }
    
    return true;
}

bool Mp3Adapter::save(const std::string& filePath, const std::vector<float>& samples) {
    if (samples.empty()) {
        if (logger_) {
            logger_->error("Cannot save: empty sample buffer");
        }
        return false;
    }
    
    if (channels_ <= 0) {
        if (logger_) {
            logger_->error("Cannot save: invalid channel count (" + 
                          std::to_string(channels_) + ")");
        }
        return false;
    }
    
    if (sampleRate_ <= 0) {
        if (logger_) {
            logger_->error("Cannot save: invalid sample rate (" + 
                          std::to_string(sampleRate_) + ")");
        }
        return false;
    }
    
    if (logger_) {
        logger_->log("Saving MP3: " + filePath);
        
        float maxSample = 0.0f;
        double sumSquares = 0.0;
        for (const float sample : samples) {
            maxSample = std::max(maxSample, std::abs(sample));
            sumSquares += sample * sample;
        }
        const double rms = std::sqrt(sumSquares / samples.size());
        logger_->log("Saving samples - Count: " + std::to_string(samples.size()) +
                     ", Max: " + std::to_string(maxSample) + 
                     ", RMS: " + std::to_string(rms));
    }
    
    lame_t lame = lame_init();
    if (!lame) {
        if (logger_) {
            logger_->error("Failed to initialize LAME");
        }
        return false;
    }
    
    lame_set_in_samplerate(lame, sampleRate_);
    lame_set_num_channels(lame, channels_);
    lame_set_quality(lame, 2);
    
    if (lame_init_params(lame) < 0) {
        if (logger_) {
            logger_->error("Failed to initialize LAME parameters");
        }
        lame_close(lame);
        return false;
    }

    FILE* file = fopen(filePath.c_str(), "wb");
    if (!file) {
        if (logger_) {
            logger_->error("Failed to open output file: " + filePath);
        }
        lame_close(lame);
        return false;
    }

    std::vector<short> intSamples(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        const float clamped = std::clamp(samples[i], -1.0f, 1.0f);
        intSamples[i] = static_cast<short>(clamped * audio::kMaxSampleValue);
    }

    const size_t mp3BufferSize = audio::kMp3WriteBufferMultiplier + 
                                  static_cast<size_t>(samples.size() / channels_ * 1.25);
    std::vector<unsigned char> mp3Buffer(mp3BufferSize);
    
    int mp3Size = 0;
    if (channels_ == 2) {
        mp3Size = lame_encode_buffer_interleaved(
            lame, 
            intSamples.data(),
            static_cast<int>(samples.size() / channels_), 
            mp3Buffer.data(), 
            static_cast<int>(mp3Buffer.size())
        );
    } else {
        mp3Size = lame_encode_buffer(
            lame, 
            intSamples.data(), 
            nullptr,
            static_cast<int>(samples.size()), 
            mp3Buffer.data(), 
            static_cast<int>(mp3Buffer.size())
        );
    }
    
    if (mp3Size < 0) {
        if (logger_) {
            logger_->error("LAME encoding failed with error: " + std::to_string(mp3Size));
        }
        fclose(file);
        lame_close(lame);
        return false;
    }
    
    fwrite(mp3Buffer.data(), 1, static_cast<size_t>(mp3Size), file);

    mp3Size = lame_encode_flush(lame, mp3Buffer.data(), static_cast<int>(mp3Buffer.size()));
    if (mp3Size > 0) {
        fwrite(mp3Buffer.data(), 1, static_cast<size_t>(mp3Size), file);
    }

    fclose(file);
    lame_close(lame);
    
    if (logger_) {
        logger_->log("Successfully saved MP3: " + filePath);
    }
    
    return true;
}