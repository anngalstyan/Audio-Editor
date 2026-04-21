#pragma once
#include <array>

namespace audio {

constexpr int kDefaultSampleRate = 44100;
constexpr int kDefaultChannels = 2;
constexpr int kBitsPerSample = 16;
constexpr float kMaxSampleValue = 32767.0f;
constexpr float kSampleNormalizationFactor = 32768.0f;

constexpr size_t kMp3ReadBufferSize = 4096;
constexpr size_t kMp3WriteBufferMultiplier = 7200;

namespace reverb {
    constexpr int kNumCombFilters = 4;
    constexpr int kNumAllpassFilters = 2;
    
    constexpr std::array<int, 4> kCombDelays = {1116, 1188, 1277, 1356};
    constexpr std::array<int, 2> kAllpassDelays = {556, 441};
    
    constexpr float kAllpassGain = 0.5f;
    
    constexpr float kMinFeedback = 0.30f;
    constexpr float kMaxFeedback = 0.85f;
    
    constexpr float kMinWetMix = 0.20f;
    constexpr float kMaxWetMix = 1.0f;
    
    constexpr float kMinDamping = 0.15f;
    constexpr float kMaxDamping = 0.5f;
}

namespace speed {
    constexpr float kMinSpeedFactor = 0.1f;
    constexpr float kMaxSpeedFactor = 2.0f;
    constexpr float kDefaultSpeedFactor = 1.0f;
}

namespace volume {
    constexpr float kMinGain = 0.0f;
    constexpr float kMaxGain = 2.0f;
    constexpr float kDefaultGain = 1.0f;
    constexpr float kSoftClipThreshold = 1.0f;
}

namespace normalize {
    constexpr float kDefaultTargetRMS = 0.15f;
    constexpr float kDefaultTargetPeak = 0.95f;
    constexpr float kMinRMSThreshold = 0.0001f;
}

namespace ui {
    constexpr int kPreviewDebounceMs = 150;
    constexpr int kPositionUpdateMs = 50;
    constexpr int kParameterDebounceMs = 80;
    constexpr int kStatusMessageDurationMs = 3000;
    constexpr int kStatusMessageShortMs = 1500;
    constexpr int kStatusMessageLongMs = 5000;
}

namespace waveform {
    constexpr float kMinZoom = 1.0f;
    constexpr float kMaxZoom = 50.0f;
    constexpr float kZoomFactor = 1.2f;
    constexpr float kAutoScrollThreshold = 0.8f;
    constexpr float kAutoScrollPosition = 0.2f;
    constexpr int kPeakSamplingStep = 100;
    constexpr float kDisplayPadding = 0.9f;
}

}