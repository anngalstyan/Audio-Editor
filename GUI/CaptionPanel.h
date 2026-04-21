#ifndef CAPTION_PANEL_H
#define CAPTION_PANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <vector>
#include <memory>
#include "../Core/Caption.h"

class ILogger;

class CaptionPanel : public QWidget {
    Q_OBJECT

public:
    explicit CaptionPanel(std::shared_ptr<ILogger> logger, QWidget* parent = nullptr);
    ~CaptionPanel() = default;

    void setCaptions(const std::vector<Caption>& captions);

    std::vector<Caption> getCaptions() const;

    void clearCaptions();

    void setCurrentTime(qint64 positionMs);

    void setEnabled(bool enabled);

    void setSpeedFactor(float factor) { speedFactor_ = factor; }

signals:

    void importRequested();

    void exportRequested();

    void seekRequested(qint64 positionMs);

private slots:
    void onImportClicked();
    void onExportClicked();
    void onCaptionClicked(int row, int column);
    void onCaptionDoubleClicked(int row, int column);

private:
    void setupUI();
    void updateTable();
    void highlightRow(int row);
    QString formatTime(double seconds);

    std::shared_ptr<ILogger> logger_;
    
    // Captions data
    std::vector<Caption> captions_;
    int currentHighlightedRow_;
    float speedFactor_ = 1.0f;
    
    // UI Components
    QVBoxLayout* mainLayout_;
    QLabel* headerLabel_;
    QTableWidget* captionTable_;
    QPushButton* importButton_;
    QPushButton* exportButton_;
    QLabel* noCaptionsLabel_;
    QLabel* currentCaptionLabel_;
};

#endif // CAPTION_PANEL_H