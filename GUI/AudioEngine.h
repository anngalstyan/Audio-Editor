#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <QObject>
#include <QAudioSink>
#include <QAudioFormat>
#include <QBuffer>
#include <QTimer>
#include <QByteArray>
#include <memory>
#include <vector>
#include "../Core/Effects/IEffect.h"

class AudioClip;

enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

class AudioEngine : public QObject {
    Q_OBJECT

public:
    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine() override;

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    void setAudioClip(std::shared_ptr<AudioClip> clip);

     std::shared_ptr<AudioClip> getAudioClip() const;

    void play();
    void pause();
    void stop();
    void seek(qint64 positionMs);

     PlaybackState getState() const;
     qint64 getPositionMs() const;
     qint64 getDurationMs() const;
     float getVolume() const;
    
     int getSampleRate() const { return sampleRate_; }
     int getChannels() const { return channels_; }

    void setOriginalSamples(const std::vector<float>& samples);

    void previewWithEffects(const std::vector<std::shared_ptr<IEffect>>& effects);

    void previewWithSamples(const std::vector<float>& samples);

    void commitEffects();

    void revertToOriginal();

     bool hasPreview() const { return hasPreview_; }

    void setVolume(float volume);

signals:
    void positionChanged(qint64 positionMs);
    void stateChanged(PlaybackState state);
    void durationChanged(qint64 durationMs);
    void audioFinished();

private slots:
    void onAudioStateChanged(QAudio::State state);
    void updatePosition();

private:
    void setupAudio();
    void convertSamplesToBytes();
    void applyVolume(QByteArray& buffer);

    // Non-destructive editing buffers
    std::vector<float> originalSamples_;
    std::vector<float> previewSamples_;
    bool hasPreview_;

    std::shared_ptr<AudioClip> audioClip_;
    
    QAudioSink* audioSink_;
    QBuffer* audioBuffer_;
    QByteArray audioData_;
    QTimer* positionTimer_;
    
    PlaybackState state_;
    float volume_;
    int sampleRate_;
    int channels_;
    qint64 pausedPosition_;
};

#endif