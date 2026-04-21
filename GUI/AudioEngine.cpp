#include "AudioEngine.h"
#include "../Core/AudioClip.h"
#include "../Core/Effects/Reverb.h"
#include "../Core/Effects/Speed.h"
#include <QMediaDevices>
#include <QAudioDevice>
#include <QtEndian>
#include <QDebug>
#include <cmath>
#include <algorithm>

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
    , audioClip_(nullptr)
    , audioSink_(nullptr)
    , audioBuffer_(nullptr)
    , positionTimer_(new QTimer(this))
    , state_(PlaybackState::Stopped)
    , volume_(1.0f)
    , sampleRate_(44100)
    , channels_(2)
    , pausedPosition_(0)
    , hasPreview_(false)
{
    connect(positionTimer_, &QTimer::timeout, this, &AudioEngine::updatePosition);
}

AudioEngine::~AudioEngine() {
    stop();
    delete audioSink_;
    delete audioBuffer_;
}

void AudioEngine::setAudioClip(std::shared_ptr<AudioClip> clip) {
    stop();
    
    audioClip_ = clip;
    
    if (audioClip_ && audioClip_->isLoaded()) {
        sampleRate_ = audioClip_->getSampleRate();
        channels_ = audioClip_->getChannels();
        
        originalSamples_ = audioClip_->getSamples();
        previewSamples_.clear();
        hasPreview_ = false;
        
        convertSamplesToBytes();
        setupAudio();
        
        emit durationChanged(getDurationMs());
    } else {
        originalSamples_.clear();
        previewSamples_.clear();
        hasPreview_ = false;
        audioData_.clear();
        sampleRate_ = 44100;
        channels_ = 2;
    }
}

std::shared_ptr<AudioClip> AudioEngine::getAudioClip() const {
    return audioClip_;
}

void AudioEngine::setupAudio() {
    if (audioSink_) {
        audioSink_->stop();
        delete audioSink_;
        audioSink_ = nullptr;
    }
    
    if (audioBuffer_) {
        delete audioBuffer_;
        audioBuffer_ = nullptr;
    }
    
    QAudioFormat format;
    format.setSampleRate(sampleRate_);
    format.setChannelCount(channels_);
    format.setSampleFormat(QAudioFormat::Int16);
    
    QAudioDevice audioDevice = QMediaDevices::defaultAudioOutput();
    
    if (audioDevice.isNull()) {
        qWarning() << "No audio output device available";
        return;
    }
    
    if (!audioDevice.isFormatSupported(format)) {
        qWarning() << "Audio format not supported, trying nearest format";
        format = audioDevice.preferredFormat();
    }
    
    audioSink_ = new QAudioSink(audioDevice, format, this);
    
    connect(audioSink_, &QAudioSink::stateChanged, 
            this, &AudioEngine::onAudioStateChanged);
    
    audioBuffer_ = new QBuffer(&audioData_, this);
}

void AudioEngine::convertSamplesToBytes() {
    if (!audioClip_ || audioClip_->getSamples().empty()) {
        audioData_.clear();
        return;
    }
    
    const std::vector<float>& samples = audioClip_->getSamples();
    
    audioData_.resize(static_cast<qsizetype>(samples.size() * sizeof(qint16)));
    qint16* dataPtr = reinterpret_cast<qint16*>(audioData_.data());
    
    for (size_t i = 0; i < samples.size(); ++i) {
        float sample = std::clamp(samples[i], -1.0f, 1.0f);
        dataPtr[i] = static_cast<qint16>(sample * 32767.0f);
    }
}

void AudioEngine::play() {
    if (!audioSink_ || audioData_.isEmpty()) {
        qWarning() << "Cannot play: no audio loaded";
        return;
    }
    
    if (state_ == PlaybackState::Playing) {
        return;
    }
    
    if (state_ == PlaybackState::Paused) {
        audioBuffer_->open(QIODevice::ReadOnly);
        audioBuffer_->seek(pausedPosition_);
    } else {
        audioBuffer_->open(QIODevice::ReadOnly);
        audioBuffer_->seek(0);
    }
    
    audioSink_->start(audioBuffer_);
    
    state_ = PlaybackState::Playing;
    emit stateChanged(state_);
    
    positionTimer_->start(50);
}

void AudioEngine::pause() {
    if (state_ != PlaybackState::Playing) {
        return;
    }
    
    pausedPosition_ = audioBuffer_->pos();
    
    audioSink_->stop();
    audioBuffer_->close();
    
    state_ = PlaybackState::Paused;
    emit stateChanged(state_);
    
    positionTimer_->stop();
}

void AudioEngine::stop() {
    if (state_ == PlaybackState::Stopped) {
        return;
    }
    
    if (audioSink_) {
        audioSink_->stop();
    }
    
    if (audioBuffer_ && audioBuffer_->isOpen()) {
        audioBuffer_->close();
    }
    
    pausedPosition_ = 0;
    
    state_ = PlaybackState::Stopped;
    emit stateChanged(state_);
    emit positionChanged(0);
    
    positionTimer_->stop();
}

void AudioEngine::seek(qint64 positionMs) {
    if (!audioBuffer_ || audioData_.isEmpty()) {
        return;
    }
    
    qint64 bytePosition = (positionMs * sampleRate_ * channels_ * static_cast<qint64>(sizeof(qint16))) / 1000;

    int frameSize = channels_ * static_cast<int>(sizeof(qint16));
    bytePosition = (bytePosition / frameSize) * frameSize;
    
    bytePosition = std::clamp(bytePosition, qint64(0), qint64(audioData_.size()));
    
    if (state_ == PlaybackState::Playing) {
        audioSink_->stop();
        audioBuffer_->close();
        audioBuffer_->open(QIODevice::ReadOnly);
        audioBuffer_->seek(bytePosition);
        audioSink_->start(audioBuffer_);
    } else {
        pausedPosition_ = bytePosition;
    }
    
    emit positionChanged(positionMs);
}

PlaybackState AudioEngine::getState() const {
    return state_;
}

qint64 AudioEngine::getPositionMs() const {
    if (!audioBuffer_ || audioData_.isEmpty()) {
        return 0;
    }
    
    qint64 bytePos;
    
    if (state_ == PlaybackState::Playing && audioBuffer_->isOpen()) {
        bytePos = audioBuffer_->pos();
    } else {
        bytePos = pausedPosition_;
    }
    
    qint64 positionMs = (bytePos * 1000) / (sampleRate_ * channels_ * static_cast<qint64>(sizeof(qint16)));
    
    return positionMs;
}

qint64 AudioEngine::getDurationMs() const {
    if (audioData_.isEmpty()) {
        return 0;
    }
    
    qint64 durationMs = (audioData_.size() * 1000) / 
                        (sampleRate_ * channels_ * static_cast<qint64>(sizeof(qint16)));
    
    return durationMs;
}

float AudioEngine::getVolume() const {
    return volume_;
}

void AudioEngine::setVolume(float volume) {
    volume_ = std::clamp(volume, 0.0f, 1.0f);
    
    if (audioSink_) {
        audioSink_->setVolume(volume_);
    }
}

void AudioEngine::onAudioStateChanged(QAudio::State audioState) {
    switch (audioState) {
        case QAudio::IdleState:
            if (state_ == PlaybackState::Playing) {
                stop();
                emit audioFinished();
            }
            break;
            
        case QAudio::StoppedState:
            if (audioSink_ && audioSink_->error() != QAudio::NoError) {
                qWarning() << "Audio error:" << audioSink_->error();
                stop();
            }
            break;
            
        case QAudio::ActiveState:
            break;
            
        case QAudio::SuspendedState:
            break;
    }
}

void AudioEngine::updatePosition() {
    if (state_ == PlaybackState::Playing) {
        emit positionChanged(getPositionMs());
    }
}

void AudioEngine::setOriginalSamples(const std::vector<float>& samples) {
    originalSamples_ = samples;
    previewSamples_.clear();
    hasPreview_ = false;
}

void AudioEngine::previewWithEffects(const std::vector<std::shared_ptr<IEffect>>& effects) {
    if (originalSamples_.empty()) {
        qWarning() << "previewWithEffects: No original samples loaded";
        return;
    }
    
    if (effects.empty()) {
        revertToOriginal();
        return;
    }
    
    previewSamples_ = originalSamples_;
    
    for (const auto& effect : effects) {
        if (!effect) continue;
        
        if (auto* reverb = dynamic_cast<Reverb*>(effect.get())) {
            reverb->reset();
        }
        
        effect->apply(previewSamples_);
    }
    
    hasPreview_ = true;
    
    // Convert preview samples to bytes for playback
    audioData_.resize(static_cast<qsizetype>(previewSamples_.size() * sizeof(qint16)));
    qint16* dataPtr = reinterpret_cast<qint16*>(audioData_.data());
    for (size_t i = 0; i < previewSamples_.size(); ++i) {
        float sample = std::clamp(previewSamples_[i], -1.0f, 1.0f);
        dataPtr[i] = static_cast<qint16>(sample * 32767.0f);
    }
    
    emit durationChanged(getDurationMs());
}

void AudioEngine::previewWithSamples(const std::vector<float>& samples) {
    if (samples.empty()) {
        revertToOriginal();
        return;
    }

    previewSamples_ = samples;
    hasPreview_ = true;

    audioData_.resize(static_cast<qsizetype>(previewSamples_.size() * sizeof(qint16)));
    qint16* dataPtr = reinterpret_cast<qint16*>(audioData_.data());
    for (size_t i = 0; i < previewSamples_.size(); ++i) {
        float sample = std::clamp(previewSamples_[i], -1.0f, 1.0f);
        dataPtr[i] = static_cast<qint16>(sample * 32767.0f);
    }

    emit durationChanged(getDurationMs());
}

void AudioEngine::commitEffects() {
    if (hasPreview_ && !previewSamples_.empty()) {
        originalSamples_ = previewSamples_;
        hasPreview_ = false;
    }
}

void AudioEngine::revertToOriginal() {
    if (originalSamples_.empty()) {
        return;
    }
    
    previewSamples_.clear();
    hasPreview_ = false;
    
    audioData_.resize(static_cast<qsizetype>(originalSamples_.size() * sizeof(qint16)));
    qint16* dataPtr = reinterpret_cast<qint16*>(audioData_.data());
    for (size_t i = 0; i < originalSamples_.size(); ++i) {
        float sample = std::clamp(originalSamples_[i], -1.0f, 1.0f);
        dataPtr[i] = static_cast<qint16>(sample * 32767.0f);
    }
    
    emit durationChanged(getDurationMs());
}
