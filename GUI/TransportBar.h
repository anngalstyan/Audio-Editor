#ifndef TRANSPORT_BAR_H
#define TRANSPORT_BAR_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QToolButton>

enum class PlaybackState;

class TransportBar : public QWidget {
    Q_OBJECT

public:
    explicit TransportBar(QWidget* parent = nullptr);
    ~TransportBar() = default;

    // Called by AudioEngine signals
    void setPosition(qint64 positionMs);
    void setDuration(qint64 durationMs);
    void setState(PlaybackState state);
    void setVolume(float volume);

signals:
    // Emitted when user clicks controls
    void playClicked();
    void pauseClicked();
    void stopClicked();
    void seekRequested(qint64 positionMs);
    void volumeChanged(float volume);

private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onSliderPressed();
    void onSliderReleased();
    void onSliderMoved(int value);
    void onVolumeSliderChanged(int value);

private:
    void setupUI();
    void updateTimeLabel();
    QString formatTime(qint64 ms);

    // Widgets
    QPushButton* playPauseButton_;
    QPushButton* stopButton_;
    QSlider* timeSlider_;
    QLabel* timeLabel_;
    QSlider* volumeSlider_;
    QLabel* volumeIcon_;

    // State
    PlaybackState currentState_;
    qint64 currentPositionMs_;
    qint64 totalDurationMs_;
    bool isSliderBeingDragged_;
};

#endif // TRANSPORT_BAR_H