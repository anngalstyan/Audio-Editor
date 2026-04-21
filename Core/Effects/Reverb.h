#ifndef REVERB_H
#define REVERB_H

#include "IEffect.h"
#include "../Logging/ILogger.h"
#include "../Constants.h"
#include <memory>
#include <vector>
#include <array>

class Reverb : public IEffect {
public:
    explicit Reverb(std::shared_ptr<ILogger> logger = nullptr);
    ~Reverb() override = default;
    
    // Disable copy (has internal state buffers)
    Reverb(const Reverb&) = delete;
    Reverb& operator=(const Reverb&) = delete;
    
    Reverb(Reverb&&) = default;
    Reverb& operator=(Reverb&&) = default;
    
    void apply(std::vector<float>& audioBuffer) override;
     std::string getName() const noexcept override { return "Reverb"; }
    
    void setIntensity(float intensity);
     float getIntensity() const { return intensity_; }
    void setParameter(const std::string& name, float value) override;

     std::shared_ptr<IEffect> clone() const override;
    void reset();

private:
    float intensity_;
    std::shared_ptr<ILogger> logger_;
    
    // Comb filter state (stereo)
    std::array<std::vector<float>, audio::reverb::kNumCombFilters> combBuffersL_;
    std::array<std::vector<float>, audio::reverb::kNumCombFilters> combBuffersR_;
    std::array<size_t, audio::reverb::kNumCombFilters> combIndices_;
    std::array<float, audio::reverb::kNumCombFilters> combFilterStore_;
    
    // Allpass filter state (stereo)
    std::array<std::vector<float>, audio::reverb::kNumAllpassFilters> allpassBuffersL_;
    std::array<std::vector<float>, audio::reverb::kNumAllpassFilters> allpassBuffersR_;
    std::array<size_t, audio::reverb::kNumAllpassFilters> allpassIndices_;
    
    bool initialized_;
    void initBuffers(int sampleRate);
};

#endif