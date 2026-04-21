#include "CaptionPanel.h"
#include "../Core/Logging/ILogger.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>

CaptionPanel::CaptionPanel(std::shared_ptr<ILogger> logger, QWidget* parent)
    : QWidget(parent)
    , logger_(logger)
    , currentHighlightedRow_(-1)
    , mainLayout_(nullptr)
    , headerLabel_(nullptr)
    , captionTable_(nullptr)
    , importButton_(nullptr)
    , exportButton_(nullptr)
    , noCaptionsLabel_(nullptr)
    , currentCaptionLabel_(nullptr)
{
    setupUI();
}

void CaptionPanel::setupUI() {
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(10, 10, 10, 10);
    mainLayout_->setSpacing(10);

    headerLabel_ = new QLabel("Captions", this);
    headerLabel_->setStyleSheet(R"(
        QLabel {
            color: #00bcd4;
            font-size: 16px;
            font-weight: bold;
            padding: 5px 0;
        }
    )");
    mainLayout_->addWidget(headerLabel_);

    currentCaptionLabel_ = new QLabel(this);
    currentCaptionLabel_->setWordWrap(true);
    currentCaptionLabel_->setAlignment(Qt::AlignCenter);
    currentCaptionLabel_->setMinimumHeight(60);
    currentCaptionLabel_->setStyleSheet(R"(
        QLabel {
            background-color: #2d2d2d;
            color: #ffffff;
            font-size: 14px;
            padding: 15px;
            border-radius: 6px;
            border: 1px solid #3d3d3d;
        }
    )");
    currentCaptionLabel_->setText("No caption");
    mainLayout_->addWidget(currentCaptionLabel_);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);

    importButton_ = new QPushButton("Import SRT", this);
    importButton_->setStyleSheet(R"(
        QPushButton {
            background-color: #4caf50;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 15px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #66bb6a;
        }
        QPushButton:pressed {
            background-color: #388e3c;
        }
    )");
    connect(importButton_, &QPushButton::clicked, this, &CaptionPanel::onImportClicked);

    exportButton_ = new QPushButton("Export TXT", this);
    exportButton_->setEnabled(false);
    exportButton_->setStyleSheet(R"(
        QPushButton {
            background-color: #3d3d3d;
            color: #e0e0e0;
            border: none;
            border-radius: 4px;
            padding: 8px 15px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #4d4d4d;
        }
        QPushButton:pressed {
            background-color: #2d2d2d;
        }
        QPushButton:disabled {
            background-color: #2a2a2a;
            color: #555555;
        }
    )");
    connect(exportButton_, &QPushButton::clicked, this, &CaptionPanel::onExportClicked);

    buttonLayout->addWidget(importButton_);
    buttonLayout->addWidget(exportButton_);
    mainLayout_->addLayout(buttonLayout);

    captionTable_ = new QTableWidget(this);
    captionTable_->setColumnCount(3);
    captionTable_->setHorizontalHeaderLabels({"Start", "End", "Text"});
    captionTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    captionTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    captionTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    captionTable_->setAlternatingRowColors(true);
    captionTable_->verticalHeader()->setVisible(false);
    captionTable_->setShowGrid(false);
    
    captionTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    captionTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    captionTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    captionTable_->setColumnWidth(0, 60);
    captionTable_->setColumnWidth(1, 60);
    
    captionTable_->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e1e1e;
            color: #e0e0e0;
            border: 1px solid #3d3d3d;
            border-radius: 4px;
            gridline-color: transparent;
        }
        QTableWidget::item {
            padding: 8px 5px;
            border-bottom: 1px solid #2d2d2d;
        }
        QTableWidget::item:selected {
            background-color: #00bcd4;
            color: #000000;
        }
        QTableWidget::item:alternate {
            background-color: #252525;
        }
        QHeaderView::section {
            background-color: #2d2d2d;
            color: #a0a0a0;
            padding: 8px 5px;
            border: none;
            border-bottom: 2px solid #3d3d3d;
            font-weight: bold;
            font-size: 11px;
        }
        QScrollBar:vertical {
            background-color: #1e1e1e;
            width: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background-color: #555555;
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #666666;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
    )");
    
    connect(captionTable_, &QTableWidget::cellClicked, 
            this, &CaptionPanel::onCaptionClicked);
    connect(captionTable_, &QTableWidget::cellDoubleClicked,
            this, &CaptionPanel::onCaptionDoubleClicked);

    noCaptionsLabel_ = new QLabel("No captions loaded.\nClick 'Import SRT' to load a subtitle file.", this);
    noCaptionsLabel_->setAlignment(Qt::AlignCenter);
    noCaptionsLabel_->setStyleSheet(R"(
        QLabel {
            color: #808080;
            font-size: 12px;
            padding: 30px;
            background-color: #1e1e1e;
            border: 1px solid #3d3d3d;
            border-radius: 4px;
        }
    )");

    QWidget* tableContainer = new QWidget(this);
    QVBoxLayout* tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->addWidget(captionTable_);
    
    mainLayout_->addWidget(tableContainer, 1);
    
    noCaptionsLabel_->setParent(tableContainer);
    captionTable_->hide();

    setStyleSheet(R"(
        CaptionPanel {
            background-color: #252525;
            border-left: 1px solid #3d3d3d;
        }
    )");
    
    setMinimumWidth(280);
    setMaximumWidth(400);
}

void CaptionPanel::setCaptions(const std::vector<Caption>& captions) {
    captions_ = captions;
    currentHighlightedRow_ = -1;
    
    updateTable();
    
    bool hasCaptions = !captions_.empty();
    exportButton_->setEnabled(hasCaptions);
    captionTable_->setVisible(hasCaptions);
    noCaptionsLabel_->setVisible(!hasCaptions);
    
    if (hasCaptions) {
        currentCaptionLabel_->setText("Ready");
    } else {
        currentCaptionLabel_->setText("No caption");
    }
    
    if (logger_) {
        logger_->log("Loaded " + std::to_string(captions_.size()) + " captions");
    }
}

std::vector<Caption> CaptionPanel::getCaptions() const {
    return captions_;
}

void CaptionPanel::clearCaptions() {
    captions_.clear();
    currentHighlightedRow_ = -1;
    captionTable_->setRowCount(0);
    
    exportButton_->setEnabled(false);
    captionTable_->hide();
    noCaptionsLabel_->show();
    currentCaptionLabel_->setText("No caption");
}

void CaptionPanel::updateTable() {
    captionTable_->setRowCount(static_cast<int>(captions_.size()));
    
    for (size_t i = 0; i < captions_.size(); ++i) {
        const Caption& cap = captions_[i];
        int row = static_cast<int>(i);
        
        QTableWidgetItem* startItem = new QTableWidgetItem(formatTime(cap.startTime));
        startItem->setTextAlignment(Qt::AlignCenter);
        captionTable_->setItem(row, 0, startItem);
        
        QTableWidgetItem* endItem = new QTableWidgetItem(formatTime(cap.endTime));
        endItem->setTextAlignment(Qt::AlignCenter);
        captionTable_->setItem(row, 1, endItem);
        
        QTableWidgetItem* textItem = new QTableWidgetItem(cap.text);
        captionTable_->setItem(row, 2, textItem);
    }
}

void CaptionPanel::setCurrentTime(qint64 positionMs) {
    float safeSpeedFactor = speedFactor_;
    if (safeSpeedFactor <= 0.01f || safeSpeedFactor > 10.0f) {
        safeSpeedFactor = 1.0f;
    }
    
    double positionSec = (positionMs / 1000.0) * safeSpeedFactor;
    
    int foundRow = -1;
    for (size_t i = 0; i < captions_.size(); ++i) {
        if (positionSec >= captions_[i].startTime && 
            positionSec < captions_[i].endTime) {
            foundRow = static_cast<int>(i);
            break;
        }
    }
    
    if (foundRow != currentHighlightedRow_) {
        highlightRow(foundRow);
        currentHighlightedRow_ = foundRow;
        
        if (foundRow >= 0 && foundRow < static_cast<int>(captions_.size())) {
            currentCaptionLabel_->setText(captions_[foundRow].text);
            currentCaptionLabel_->setStyleSheet(R"(
                QLabel {
                    background-color: #00bcd4;
                    color: #000000;
                    font-size: 14px;
                    font-weight: bold;
                    padding: 15px;
                    border-radius: 6px;
                    border: none;
                }
            )");
        } else {
            currentCaptionLabel_->setText("");
            currentCaptionLabel_->setStyleSheet(R"(
                QLabel {
                    background-color: #2d2d2d;
                    color: #ffffff;
                    font-size: 14px;
                    padding: 15px;
                    border-radius: 6px;
                    border: 1px solid #3d3d3d;
                }
            )");
        }
    }
}

void CaptionPanel::highlightRow(int row) {
    for (int r = 0; r < captionTable_->rowCount(); ++r) {
        for (int c = 0; c < captionTable_->columnCount(); ++c) {
            QTableWidgetItem* item = captionTable_->item(r, c);
            if (item) {
                item->setBackground(Qt::transparent);
                item->setForeground(QColor(0xe0, 0xe0, 0xe0));
            }
        }
    }
    
    if (row >= 0 && row < captionTable_->rowCount()) {
        for (int c = 0; c < captionTable_->columnCount(); ++c) {
            QTableWidgetItem* item = captionTable_->item(row, c);
            if (item) {
                item->setBackground(QColor(0x00, 0xbc, 0xd4));
                item->setForeground(QColor(0x00, 0x00, 0x00));
            }
        }
        
        captionTable_->scrollToItem(captionTable_->item(row, 0),
                                     QAbstractItemView::PositionAtCenter);
    }
}

void CaptionPanel::setEnabled(bool enabled) {
    importButton_->setEnabled(enabled);
    exportButton_->setEnabled(enabled && !captions_.empty());
    captionTable_->setEnabled(enabled);
}

void CaptionPanel::onImportClicked() {
    emit importRequested();
}

void CaptionPanel::onExportClicked() {
    emit exportRequested();
}

void CaptionPanel::onCaptionClicked(int row, int column) {
    Q_UNUSED(column);
    
    if (row >= 0 && row < static_cast<int>(captions_.size())) {
        qint64 positionMs = static_cast<qint64>(captions_[row].startTime * 1000);
        emit seekRequested(positionMs);
    }
}

void CaptionPanel::onCaptionDoubleClicked(int row, int column) {
    onCaptionClicked(row, column);
}

QString CaptionPanel::formatTime(double seconds) {
    int totalSeconds = static_cast<int>(seconds);
    int minutes = totalSeconds / 60;
    int secs = totalSeconds % 60;
    
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}