#include "TransportBar.h"
#include "AudioEngine.h"
#include <QHBoxLayout>
#include <QStyle>
#include <QApplication>
#include <QMouseEvent>

// Custom slider that supports click-to-seek
class ClickableSlider : public QSlider {
public:
    ClickableSlider(Qt::Orientation orientation, QWidget* parent = nullptr)
        : QSlider(orientation, parent) {}

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            // Calculate position from click
            int value;
            if (orientation() == Qt::Horizontal) {
                value = minimum() + ((maximum() - minimum()) * event->pos().x()) / width();
            } else {
                value = minimum() + ((maximum() - minimum()) * (height() - event->pos().y())) / height();
            }
            setValue(value);
            emit sliderMoved(value);
            event->accept();
        }
        QSlider::mousePressEvent(event);
    }
};

TransportBar::TransportBar(QWidget* parent)
    : QWidget(parent)
    , playPauseButton_(nullptr)
    , stopButton_(nullptr)
    , timeSlider_(nullptr)
    , timeLabel_(nullptr)
    , volumeSlider_(nullptr)
    , volumeIcon_(nullptr)
    , currentState_(PlaybackState::Stopped)
    , currentPositionMs_(0)
    , totalDurationMs_(0)
    , isSliderBeingDragged_(false)
{
    setupUI();
}

void TransportBar::setupUI() {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->setSpacing(10);

    // Play/Pause Button
    playPauseButton_ = new QPushButton(this);
    playPauseButton_->setFixedSize(40, 40);
    playPauseButton_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    playPauseButton_->setIconSize(QSize(20, 20));
    playPauseButton_->setToolTip("Play (Space)");
    playPauseButton_->setStyleSheet(R"(
        QPushButton {
            background-color: #00bcd4;
            border: none;
            border-radius: 20px;
        }
        QPushButton:hover {
            background-color: #00acc1;
        }
        QPushButton:pressed {
            background-color: #0097a7;
        }
        QPushButton:disabled {
            background-color: #555555;
        }
    )");
    connect(playPauseButton_, &QPushButton::clicked, 
            this, &TransportBar::onPlayPauseClicked);

    // Stop Button
    stopButton_ = new QPushButton(this);
    stopButton_->setFixedSize(36, 36);
    stopButton_->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    stopButton_->setIconSize(QSize(18, 18));
    stopButton_->setToolTip("Stop");
    stopButton_->setStyleSheet(R"(
        QPushButton {
            background-color: #3d3d3d;
            border: none;
            border-radius: 18px;
        }
        QPushButton:hover {
            background-color: #4d4d4d;
        }
        QPushButton:pressed {
            background-color: #2d2d2d;
        }
        QPushButton:disabled {
            background-color: #2a2a2a;
        }
    )");
    connect(stopButton_, &QPushButton::clicked, 
            this, &TransportBar::onStopClicked);

    // Time Slider - using custom clickable slider
    timeSlider_ = new ClickableSlider(Qt::Horizontal, this);
    timeSlider_->setMinimum(0);
    timeSlider_->setMaximum(1000);
    timeSlider_->setValue(0);
    timeSlider_->setToolTip("Click or drag to seek");
    timeSlider_->setStyleSheet(R"(
        QSlider::groove:horizontal {
            height: 6px;
            background: #3d3d3d;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #00bcd4;
            width: 14px;
            height: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }
        QSlider::handle:horizontal:hover {
            background: #00e5ff;
        }
        QSlider::sub-page:horizontal {
            background: #00bcd4;
            border-radius: 3px;
        }
    )");
    connect(timeSlider_, &QSlider::sliderPressed, 
            this, &TransportBar::onSliderPressed);
    connect(timeSlider_, &QSlider::sliderReleased, 
            this, &TransportBar::onSliderReleased);
    connect(timeSlider_, &QSlider::sliderMoved, 
            this, &TransportBar::onSliderMoved);

    // Time Label
    timeLabel_ = new QLabel("00:00 / 00:00", this);
    timeLabel_->setFixedWidth(100);
    timeLabel_->setAlignment(Qt::AlignCenter);
    timeLabel_->setStyleSheet("QLabel { color: #e0e0e0; }");

    // Volume Icon
    volumeIcon_ = new QLabel(this);
    volumeIcon_->setPixmap(style()->standardIcon(QStyle::SP_MediaVolume).pixmap(18, 18));
    volumeIcon_->setFixedSize(18, 18);

    // Volume Slider
    volumeSlider_ = new QSlider(Qt::Horizontal, this);
    volumeSlider_->setMinimum(0);
    volumeSlider_->setMaximum(100);
    volumeSlider_->setValue(100);
    volumeSlider_->setFixedWidth(80);
    volumeSlider_->setToolTip("Volume");
    volumeSlider_->setStyleSheet(R"(
        QSlider::groove:horizontal {
            height: 4px;
            background: #3d3d3d;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #e0e0e0;
            width: 10px;
            height: 10px;
            margin: -3px 0;
            border-radius: 5px;
        }
        QSlider::sub-page:horizontal {
            background: #00bcd4;
            border-radius: 2px;
        }
    )");
    connect(volumeSlider_, &QSlider::valueChanged, 
            this, &TransportBar::onVolumeSliderChanged);

    // Add to layout
    layout->addWidget(playPauseButton_);
    layout->addWidget(stopButton_);
    layout->addWidget(timeSlider_, 1);
    layout->addWidget(timeLabel_);
    layout->addSpacing(10);
    layout->addWidget(volumeIcon_);
    layout->addWidget(volumeSlider_);

    setFixedHeight(60);
    
    setStyleSheet(R"(
        TransportBar {
            background-color: #252525;
            border-top: 1px solid #3d3d3d;
        }
    )");
}

void TransportBar::setPosition(qint64 positionMs) {
    currentPositionMs_ = positionMs;
    
    if (!isSliderBeingDragged_ && totalDurationMs_ > 0) {
        int sliderValue = static_cast<int>((positionMs * 1000) / totalDurationMs_);
        timeSlider_->blockSignals(true);
        timeSlider_->setValue(sliderValue);
        timeSlider_->blockSignals(false);
    }
    
    updateTimeLabel();
}

void TransportBar::setDuration(qint64 durationMs) {
    totalDurationMs_ = durationMs;
    updateTimeLabel();
    
    bool hasAudio = durationMs > 0;
    playPauseButton_->setEnabled(hasAudio);
    stopButton_->setEnabled(hasAudio);
    timeSlider_->setEnabled(hasAudio);
}

void TransportBar::setState(PlaybackState state) {
    currentState_ = state;
    
    switch (state) {
        case PlaybackState::Stopped:
            playPauseButton_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            playPauseButton_->setToolTip("Play (Space)");
            break;
            
        case PlaybackState::Playing:
            playPauseButton_->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
            playPauseButton_->setToolTip("Pause (Space)");
            break;
            
        case PlaybackState::Paused:
            playPauseButton_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            playPauseButton_->setToolTip("Resume (Space)");
            break;
    }
}

void TransportBar::setVolume(float volume) {
    int sliderValue = static_cast<int>(volume * 100);
    volumeSlider_->blockSignals(true);
    volumeSlider_->setValue(sliderValue);
    volumeSlider_->blockSignals(false);
}

void TransportBar::onPlayPauseClicked() {
    if (currentState_ == PlaybackState::Playing) {
        emit pauseClicked();
    } else {
        emit playClicked();
    }
}

void TransportBar::onStopClicked() {
    emit stopClicked();
}

void TransportBar::onSliderPressed() {
    isSliderBeingDragged_ = true;
}

void TransportBar::onSliderReleased() {
    isSliderBeingDragged_ = false;
    
    if (totalDurationMs_ > 0) {
        qint64 seekPosition = (timeSlider_->value() * totalDurationMs_) / 1000;
        emit seekRequested(seekPosition);
    }
}

void TransportBar::onSliderMoved(int value) {
    // Seek immediately when clicking or dragging
    if (totalDurationMs_ > 0) {
        qint64 seekPosition = (value * totalDurationMs_) / 1000;
        emit seekRequested(seekPosition);
        
        // Update time label preview
        timeLabel_->setText(formatTime(seekPosition) + " / " + formatTime(totalDurationMs_));
    }
}

void TransportBar::onVolumeSliderChanged(int value) {
    float volume = value / 100.0f;
    emit volumeChanged(volume);
    
    if (value == 0) {
        volumeIcon_->setPixmap(style()->standardIcon(QStyle::SP_MediaVolumeMuted).pixmap(18, 18));
    } else {
        volumeIcon_->setPixmap(style()->standardIcon(QStyle::SP_MediaVolume).pixmap(18, 18));
    }
}

void TransportBar::updateTimeLabel() {
    timeLabel_->setText(formatTime(currentPositionMs_) + " / " + formatTime(totalDurationMs_));
}

QString TransportBar::formatTime(qint64 ms) {
    qint64 totalSeconds = ms / 1000;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}