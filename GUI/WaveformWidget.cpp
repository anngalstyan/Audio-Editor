#include "WaveformWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>
#include <algorithm>
#include <numeric>

WaveformWidget::WaveformWidget(QWidget* parent)
    : QWidget(parent)
    , sampleRate_(44100)
    , channels_(2)
    , durationMs_(0)
    , cacheValid_(false)
    , displayScale_(1.0f)
    , zoom_(1.0f)
    , scrollOffsetMs_(0)
    , playheadPositionMs_(0)
    , isDragging_(false)
    , lastMouseX_(0)
    , backgroundColor_(QColor(0x1e, 0x1e, 0x1e))
    , waveformColor_(QColor(0x4c, 0xaf, 0x50))
    , waveformPeakColor_(QColor(0x81, 0xc7, 0x84))
    , playheadColor_(QColor(0x00, 0xbc, 0xd4))
    , centerLineColor_(QColor(0x3d, 0x3d, 0x3d))
{
    setMinimumHeight(100);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, backgroundColor_);
    setPalette(pal);
}

void WaveformWidget::setSamples(const std::vector<float>& samples, 
                                 int sampleRate, int channels) {
    samples_ = samples;
    sampleRate_ = sampleRate;
    channels_ = channels;
    
    if (sampleRate_ > 0 && channels_ > 0 && !samples_.empty()) {
        qint64 totalFrames = samples_.size() / channels_;
        durationMs_ = (totalFrames * 1000) / sampleRate_;
    } else {
        durationMs_ = 0;
    }
    
    zoom_ = 1.0f;
    scrollOffsetMs_ = 0;
    
    if (playheadPositionMs_ > durationMs_) {
        playheadPositionMs_ = 0;
    }
    
    cacheValid_ = false;
    computePeaks();
    
    update();
}

void WaveformWidget::clear() {
    samples_.clear();
    peaks_.clear();
    displayScale_ = 1.0f;
    durationMs_ = 0;
    playheadPositionMs_ = 0;
    scrollOffsetMs_ = 0;
    zoom_ = 1.0f;
    cacheValid_ = false;
    
    update();
}

void WaveformWidget::setPlayheadPosition(qint64 positionMs) {
    positionMs = std::clamp(positionMs, qint64(0), durationMs_);
    
    if (playheadPositionMs_ != positionMs) {
        playheadPositionMs_ = positionMs;
        
        if (zoom_ > 1.0f && durationMs_ > 0) {
            qint64 visibleDuration = durationMs_ / zoom_;
            
            qint64 scrollThreshold = scrollOffsetMs_ + (visibleDuration * 0.8);
            if (playheadPositionMs_ > scrollThreshold && playheadPositionMs_ < durationMs_) {
                scrollOffsetMs_ = playheadPositionMs_ - (visibleDuration * 0.2);
                scrollOffsetMs_ = std::max(qint64(0), scrollOffsetMs_);
                cacheValid_ = false;
            }
            
            if (playheadPositionMs_ < scrollOffsetMs_) {
                scrollOffsetMs_ = playheadPositionMs_;
                cacheValid_ = false;
            }
        }
        
        update();
    }
}

void WaveformWidget::setZoom(float zoom) {
    zoom_ = std::clamp(zoom, 1.0f, 50.0f);
    cacheValid_ = false;
    computePeaks();
    update();
}

float WaveformWidget::getZoom() const {
    return zoom_;
}

qint64 WaveformWidget::getDurationMs() const {
    return durationMs_;
}

void WaveformWidget::computePeaks() {
    peaks_.clear();
    
    if (samples_.empty()) {
        return;
    }
    
    int widgetWidth = width();
    if (widgetWidth <= 0) {
        return;
    }
    
    if (durationMs_ <= 0) {
        return;
    }
    
    if (sampleRate_ <= 0 || channels_ <= 0) {
        return;
    }
    
    peaks_.resize(widgetWidth);
    
    qint64 visibleDuration = durationMs_ / zoom_;
    if (visibleDuration <= 0) {
        visibleDuration = durationMs_;
    }
    
    qint64 visibleSamples = (visibleDuration * sampleRate_ * channels_) / 1000;
    if (visibleSamples <= 0) {
        return;
    }
    
    double samplesPerPixel = static_cast<double>(visibleSamples) / widgetWidth;
    if (samplesPerPixel <= 0) {
        samplesPerPixel = 1.0;
    }
    
    qint64 startSample = (scrollOffsetMs_ * sampleRate_ * channels_) / 1000;
    
    float maxAbsValue = 0.0f;

    for (int x = 0; x < widgetWidth; ++x) {
        qint64 sampleStart = startSample + static_cast<qint64>(x * samplesPerPixel);
        qint64 sampleEnd = startSample + static_cast<qint64>((x + 1) * samplesPerPixel);
        
        sampleStart = std::clamp(sampleStart, qint64(0), static_cast<qint64>(samples_.size()));
        sampleEnd = std::clamp(sampleEnd, qint64(0), static_cast<qint64>(samples_.size()));
        
        float minVal = 0.0f;
        float maxVal = 0.0f;
        
        if (sampleStart < sampleEnd) {
            minVal = samples_[sampleStart];
            maxVal = samples_[sampleStart];
            
            qint64 step = std::max(qint64(1), (sampleEnd - sampleStart) / 100);
            for (qint64 i = sampleStart; i < sampleEnd; i += step) {
                float sample = samples_[i];
                minVal = std::min(minVal, sample);
                maxVal = std::max(maxVal, sample);
            }
        }
        
        peaks_[x] = {minVal, maxVal};
        maxAbsValue = std::max(maxAbsValue, std::max(std::abs(minVal), std::abs(maxVal)));
    }

    if (maxAbsValue > 0.001f) {
        displayScale_ = 0.85f / maxAbsValue;
    } else {
        displayScale_ = 1.0f;
    }
}

void WaveformWidget::renderWaveform() {
    if (width() <= 0 || height() <= 0) {
        return;
    }
    
    waveformCache_ = QPixmap(size());
    waveformCache_.fill(backgroundColor_);
    
    QPainter painter(&waveformCache_);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setClipRect(rect());
    
    int widgetHeight = height();
    int centerY = widgetHeight / 2;
    int margin = 10;
    int drawableHeight = (widgetHeight / 2) - margin;
    
    painter.setPen(QPen(centerLineColor_, 1));
    painter.drawLine(0, centerY, width(), centerY);
    
    if (peaks_.empty()) {
        cacheValid_ = true;
        return;
    }
    
    int peakCount = static_cast<int>(peaks_.size());
    
    for (int x = 0; x < peakCount && x < width(); ++x) {
        const Peak& peak = peaks_[x];
        
        float scaledMax = std::clamp(peak.max * displayScale_, -1.0f, 1.0f);
        float scaledMin = std::clamp(peak.min * displayScale_, -1.0f, 1.0f);

        int minY = centerY - static_cast<int>(scaledMax * drawableHeight);
        int maxY = centerY - static_cast<int>(scaledMin * drawableHeight);
        
        minY = std::clamp(minY, margin, widgetHeight - margin);
        maxY = std::clamp(maxY, margin, widgetHeight - margin);
        
        if (minY == maxY) {
            maxY = minY + 1;
        }
        
        painter.setPen(QPen(waveformColor_, 1));
        painter.drawLine(x, minY, x, maxY);
        
        if (maxY - minY > 4) {
            painter.setPen(QPen(waveformPeakColor_, 1));
            painter.drawPoint(x, minY);
            painter.drawPoint(x, maxY);
        }
    }
    
    cacheValid_ = true;
}

void WaveformWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setClipRect(rect());
    
    if (!cacheValid_ || waveformCache_.size() != size()) {
        computePeaks();
        renderWaveform();
    }
    
    painter.drawPixmap(0, 0, waveformCache_);
    
    if (durationMs_ > 0) {
        int playheadX = positionToX(playheadPositionMs_);
        
        if (playheadX >= 0 && playheadX <= width()) {
            painter.setPen(QPen(playheadColor_, 2));
            painter.drawLine(playheadX, 0, playheadX, height());
            
            QPolygon triangle;
            triangle << QPoint(playheadX - 5, 0)
                     << QPoint(playheadX + 5, 0)
                     << QPoint(playheadX, 8);
            painter.setBrush(playheadColor_);
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(triangle);
        }
    }
    
    painter.setPen(QPen(QColor(0x3d, 0x3d, 0x3d), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void WaveformWidget::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event);
    cacheValid_ = false;
}

void WaveformWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && durationMs_ > 0) {
        isDragging_ = true;
        lastMouseX_ = event->pos().x();
        
        qint64 clickedPosition = xToPosition(event->pos().x());
        clickedPosition = std::clamp(clickedPosition, qint64(0), durationMs_);
        emit seekRequested(clickedPosition);
    }
}

void WaveformWidget::mouseMoveEvent(QMouseEvent* event) {
    if (isDragging_ && durationMs_ > 0) {
        qint64 dragPosition = xToPosition(event->pos().x());
        dragPosition = std::clamp(dragPosition, qint64(0), durationMs_);
        emit seekRequested(dragPosition);
    }
    
    setCursor(durationMs_ > 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void WaveformWidget::mouseReleaseEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    isDragging_ = false;
}

void WaveformWidget::wheelEvent(QWheelEvent* event) {
    if (durationMs_ <= 0) {
        return;
    }
    
    float zoomFactor = 1.2f;
    float oldZoom = zoom_;
    
    if (event->angleDelta().y() > 0) {
        zoom_ = std::min(zoom_ * zoomFactor, 50.0f);
    } else {
        zoom_ = std::max(zoom_ / zoomFactor, 1.0f);
    }
    
    if (zoom_ != oldZoom) {
        if (zoom_ > 1.0f) {
            qint64 mousePositionMs = xToPosition(event->position().x());
            qint64 visibleDuration = durationMs_ / zoom_;
            scrollOffsetMs_ = mousePositionMs - (visibleDuration / 2);
            scrollOffsetMs_ = std::clamp(scrollOffsetMs_, qint64(0), 
                                          std::max(qint64(0), durationMs_ - visibleDuration));
        } else {
            scrollOffsetMs_ = 0;
        }
        
        cacheValid_ = false;
        update();
    }
    
    event->accept();
}

int WaveformWidget::positionToX(qint64 positionMs) const {
    if (durationMs_ <= 0) {
        return 0;
    }
    
    qint64 visibleDuration = durationMs_ / zoom_;
    if (visibleDuration <= 0) return 0;
    
    qint64 relativePosition = positionMs - scrollOffsetMs_;
    
    return static_cast<int>((relativePosition * width()) / visibleDuration);
}

qint64 WaveformWidget::xToPosition(int x) const {
    if (width() <= 0 || durationMs_ <= 0) {
        return 0;
    }
    
    qint64 visibleDuration = durationMs_ / zoom_;
    qint64 positionMs = scrollOffsetMs_ + (x * visibleDuration) / width();
    
    return std::clamp(positionMs, qint64(0), durationMs_);
}
