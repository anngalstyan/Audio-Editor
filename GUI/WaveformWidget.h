#ifndef WAVEFORM_WIDGET_H
#define WAVEFORM_WIDGET_H

#include <QWidget>
#include <QPixmap>
#include <vector>

class WaveformWidget : public QWidget {
    Q_OBJECT

public:
    explicit WaveformWidget(QWidget* parent = nullptr);
    ~WaveformWidget() = default;

    void setSamples(const std::vector<float>& samples, int sampleRate, int channels);
    void clear();
    void setPlayheadPosition(qint64 positionMs);
    void setZoom(float zoom);
    float getZoom() const;
    qint64 getDurationMs() const;

signals:
    void seekRequested(qint64 positionMs);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void computePeaks();
    void renderWaveform();
    int positionToX(qint64 positionMs) const;
    qint64 xToPosition(int x) const;

    std::vector<float> samples_;
    int sampleRate_;
    int channels_;
    qint64 durationMs_;

    struct Peak {
        float min;
        float max;
    };
    std::vector<Peak> peaks_;

    QPixmap waveformCache_;
    bool cacheValid_;
    float displayScale_;

    float zoom_;
    qint64 scrollOffsetMs_;
    qint64 playheadPositionMs_;

    bool isDragging_;
    int lastMouseX_;

    QColor backgroundColor_;
    QColor waveformColor_;
    QColor waveformPeakColor_;
    QColor playheadColor_;
    QColor centerLineColor_;
};

#endif
