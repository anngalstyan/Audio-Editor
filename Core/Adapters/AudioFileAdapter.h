#pragma once

#include <vector>
#include <string>


class AudioFileAdapter {
public:
    virtual ~AudioFileAdapter() = default;

     virtual bool load(const std::string& filePath) = 0;
 
     virtual bool save(const std::string& filePath, 
                                    const std::vector<float>& samples) = 0;

     virtual const std::vector<float>& getSamples() const = 0;

     virtual float getDuration() const noexcept = 0;

     virtual int getSampleRate() const noexcept = 0;

     virtual int getChannels() const noexcept = 0;
};
