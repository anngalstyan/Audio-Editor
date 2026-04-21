#pragma once

#include <stdexcept>
#include <string>

namespace audio {

class AudioException : public std::runtime_error {
public:
    explicit AudioException(const std::string& message)
        : std::runtime_error(message) {}
};

class FileLoadException : public AudioException {
public:
    explicit FileLoadException(const std::string& filepath)
        : AudioException("Failed to load audio file: " + filepath)
        , filepath_(filepath) {}
    
     const std::string& filepath() const noexcept { return filepath_; }

private:
    std::string filepath_;
};

class FileSaveException : public AudioException {
public:
    explicit FileSaveException(const std::string& filepath)
        : AudioException("Failed to save audio file: " + filepath)
        , filepath_(filepath) {}
    
     const std::string& filepath() const noexcept { return filepath_; }

private:
    std::string filepath_;
};

class FormatException : public AudioException {
public:
    explicit FormatException(const std::string& details)
        : AudioException("Invalid audio format: " + details) {}
};

class EffectException : public AudioException {
public:
    explicit EffectException(const std::string& effectName, const std::string& details)
        : AudioException("Effect '" + effectName + "' failed: " + details)
        , effectName_(effectName) {}
    
     const std::string& effectName() const noexcept { return effectName_; }

private:
    std::string effectName_;
};

class BufferException : public AudioException {
public:
    explicit BufferException(const std::string& details)
        : AudioException("Buffer error: " + details) {}
};

}  // namespace audio
