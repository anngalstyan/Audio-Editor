#include "MainWindow.h"
#include "AudioEngine.h"
#include "TransportBar.h"
#include "WaveformWidget.h"
#include "EffectsPanel.h"
#include "CaptionPanel.h"
#include "../Core/Services/CaptionParser.h"
#include "../Core/AudioClip.h"
#include "../Core/EffectFactory.h"
#include "../Core/Commands/CommandHistory.h"
#include "../Core/Logging/CompositeLogger.h"
#include "../Core/Logging/ConsoleLogger.h"
#include "../Core/Logging/FileLogger.h"
#include "../Core/Effects/Reverb.h"
#include "../Core/Effects/Speed.h"
#include "../Core/Effects/Volume.h"
#include "../Core/Commands/ApplyEffect.h"
#include "../Core/Commands/EffectStateCommand.h"
#include <QApplication>
#include <QScreen>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QShortcut>
#include <QKeySequence>
#include <QtConcurrent/QtConcurrentRun>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , audioClip_(nullptr)
    , audioEngine_(nullptr)
    , commandHistory_(nullptr)
    , captionParser_(nullptr)
    , centralWidget_(nullptr)
    , transportBar_(nullptr)
    , waveformWidget_(nullptr)
    , effectsPanel_(nullptr)
    , captionPanel_(nullptr)
    , previewDebounceTimer_(nullptr)
    , previewWatcher_(nullptr)
    , previewComputationQueued_(false)
    , discardPreviewResult_(false)
    , hasUnsavedChanges_(false)
    , isPreviewMode_(false)
    , loadedSampleRate_(44100)
    , loadedChannels_(2)
{
    auto compositeLogger = std::make_shared<CompositeLogger>();
    compositeLogger->addLogger(std::make_shared<ConsoleLogger>());
    
    try {
        compositeLogger->addLogger(std::make_shared<FileLogger>("audioeditor.log"));
    } catch (const std::exception& e) {
        qWarning() << "Failed to create file logger:" << e.what();
    }
    
    logger_ = compositeLogger;
    logger_->log("Application starting...");
    
    // Register effect creators with factory
    EffectFactory::registerEffect("Reverb", [](std::shared_ptr<ILogger> log) {
        return std::make_shared<Reverb>(log);
    });
    EffectFactory::registerEffect("Speed", [](std::shared_ptr<ILogger> log) {
        return std::make_shared<SpeedChangeEffect>(1.0f, log);
    });
    EffectFactory::registerEffect("Volume", [](std::shared_ptr<ILogger> log) {
        return std::make_shared<VolumeEffect>(1.0f, log);
    });

    audioEngine_ = new AudioEngine(this);

    commandHistory_ = std::make_unique<CommandHistory>(logger_);
    captionParser_ = std::make_unique<CaptionParser>(logger_);

    previewDebounceTimer_ = new QTimer(this);
    previewDebounceTimer_->setSingleShot(true);
    previewDebounceTimer_->setInterval(150);
    connect(previewDebounceTimer_, &QTimer::timeout, this, &MainWindow::onPreviewTimerTimeout);

    previewWatcher_ = new QFutureWatcher<std::vector<float>>(this);
    connect(previewWatcher_, &QFutureWatcher<std::vector<float>>::finished,
            this, &MainWindow::onPreviewComputationFinished);
    
    setupUI();
    setupMenuBar();
    setupStatusBar();
    setupConnections();
    setupShortcuts();
    applyTheme();
    
    setWindowTitle("Audio Editor");
    setMinimumSize(1000, 600);

    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        int x = (screenGeometry.width() - 1200) / 2;
        int y = (screenGeometry.height() - 700) / 2;
        setGeometry(x, y, 1200, 700);
    } else {
        resize(1200, 700);
    }
    
    setAcceptDrops(true);
    updateUIState();
    
    logger_->log("Application ready");
}

MainWindow::~MainWindow() {
    logger_->log("Application closing...");
    cancelPendingPreview();
}

void MainWindow::setupShortcuts() {
    QShortcut* playPauseShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    playPauseShortcut->setContext(Qt::ApplicationShortcut);
    connect(playPauseShortcut, &QShortcut::activated, this, &MainWindow::onTogglePlayPause);
}

void MainWindow::onTogglePlayPause() {
    if (!audioClip_) return;
    
    if (audioEngine_->getState() == PlaybackState::Playing) {
        audioEngine_->pause();
    } else {
        audioEngine_->play();
    }
}

void MainWindow::setupUI() {
    centralWidget_ = new QWidget(this);
    setCentralWidget(centralWidget_);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget_);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    
    waveformWidget_ = new WaveformWidget(this);
    
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setFixedWidth(300);
    rightPanel->setStyleSheet("background-color: #252525;");
    
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    
    effectsPanel_ = new EffectsPanel(logger_, this);
    captionPanel_ = new CaptionPanel(logger_, this);
    
    QSplitter* rightSplitter = new QSplitter(Qt::Vertical, this);
    rightSplitter->addWidget(effectsPanel_);
    rightSplitter->addWidget(captionPanel_);
    rightSplitter->setSizes({300, 300});
    rightSplitter->setStyleSheet(R"(
        QSplitter::handle {
            background-color: #3d3d3d;
            height: 2px;
        }
    )");
    
    rightLayout->addWidget(rightSplitter);
    
    contentLayout->addWidget(waveformWidget_, 1);
    contentLayout->addWidget(rightPanel);
    
    transportBar_ = new TransportBar(this);
    
    mainLayout->addLayout(contentLayout, 1);
    mainLayout->addWidget(transportBar_);
}

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    fileMenu_ = menuBar->addMenu("&File");
    
    newAction_ = fileMenu_->addAction("&New Project");
    newAction_->setShortcut(QKeySequence::New);
    connect(newAction_, &QAction::triggered, this, &MainWindow::onNewProject);
    
    openAction_ = fileMenu_->addAction("&Open Audio...");
    openAction_->setShortcut(QKeySequence::Open);
    connect(openAction_, &QAction::triggered, this, &MainWindow::onOpenAudio);
    
    fileMenu_->addSeparator();
    
    saveAction_ = fileMenu_->addAction("&Save");
    saveAction_->setShortcut(QKeySequence::Save);
    saveAction_->setEnabled(false);
    connect(saveAction_, &QAction::triggered, this, &MainWindow::onSaveAudio);
    
    exportAction_ = fileMenu_->addAction("&Export As...");
    exportAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
    exportAction_->setEnabled(false);
    connect(exportAction_, &QAction::triggered, this, &MainWindow::onExportAudio);
    
    fileMenu_->addSeparator();
    
    importCaptionsAction_ = fileMenu_->addAction("Import &Captions...");
    connect(importCaptionsAction_, &QAction::triggered, this, &MainWindow::onImportCaptions);
    
    exportCaptionsAction_ = fileMenu_->addAction("Export Captions...");
    exportCaptionsAction_->setEnabled(false);
    connect(exportCaptionsAction_, &QAction::triggered, this, &MainWindow::onExportCaptions);
    
    fileMenu_->addSeparator();
    
    exitAction_ = fileMenu_->addAction("E&xit");
    exitAction_->setShortcut(QKeySequence::Quit);
    connect(exitAction_, &QAction::triggered, this, &MainWindow::onExit);
    
    editMenu_ = menuBar->addMenu("&Edit");
    
    undoAction_ = new QAction("&Undo", this);
    undoAction_->setShortcut(QKeySequence::Undo);
    undoAction_->setEnabled(false);
    connect(undoAction_, &QAction::triggered, this, &MainWindow::onUndo);
    editMenu_->addAction(undoAction_);

    redoAction_ = new QAction("&Redo", this);
    redoAction_->setShortcut(QKeySequence::Redo);
    redoAction_->setEnabled(false);
    connect(redoAction_, &QAction::triggered, this, &MainWindow::onRedo);
    editMenu_->addAction(redoAction_);
    
    helpMenu_ = menuBar->addMenu("&Help");
    
    aboutAction_ = helpMenu_->addAction("&About");
    connect(aboutAction_, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About Audio Editor",
            "<h2>Audio Editor</h2>"
            "<p>Version 1.0</p>"
            "<p><b>Design Patterns Used:</b><br>"
            "Adapter, Strategy, Command, Factory,<br>"
            "Composite, Observer (Qt signals/slots)</p>"
            "<p><b>Shortcuts:</b><br>"
            "Space - Play/Pause<br>"
            "Ctrl+Z - Undo<br>"
            "Ctrl+Shift+Z - Redo</p>"
        );
    });
    
    menuBar->setStyleSheet(R"(
        QMenuBar {
            background-color: #1e1e1e;
            color: #e0e0e0;
            padding: 5px;
        }
        QMenuBar::item {
            padding: 5px 10px;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background-color: #3d3d3d;
        }
        QMenu {
            background-color: #2d2d2d;
            color: #e0e0e0;
            border: 1px solid #3d3d3d;
            padding: 5px;
        }
        QMenu::item {
            padding: 8px 30px 8px 20px;
            border-radius: 4px;
        }
        QMenu::item:selected {
            background-color: #00bcd4;
            color: #000000;
        }
        QMenu::separator {
            height: 1px;
            background-color: #3d3d3d;
            margin: 5px 10px;
        }
    )");
}

void MainWindow::setupStatusBar() {
    QStatusBar* status = new QStatusBar(this);
    setStatusBar(status);
    
    status->setStyleSheet(R"(
        QStatusBar {
            background-color: #1e1e1e;
            color: #808080;
            border-top: 1px solid #3d3d3d;
            padding: 5px;
        }
    )");
    
    status->showMessage("Ready");
}

void MainWindow::setupConnections() {
    // Effects panel -> caption speed update
    connect(effectsPanel_, &EffectsPanel::effectsChanged,
            this, &MainWindow::updateCaptionSpeed);
    
    // Transport controls -> audio engine
    connect(transportBar_, &TransportBar::playClicked,
            audioEngine_, &AudioEngine::play);
    connect(transportBar_, &TransportBar::pauseClicked,
            audioEngine_, &AudioEngine::pause);
    connect(transportBar_, &TransportBar::stopClicked,
            audioEngine_, &AudioEngine::stop);
    connect(transportBar_, &TransportBar::seekRequested,
            audioEngine_, &AudioEngine::seek);
    connect(transportBar_, &TransportBar::volumeChanged,
            audioEngine_, &AudioEngine::setVolume);
    
    // Audio engine -> transport bar
    connect(audioEngine_, &AudioEngine::positionChanged,
            transportBar_, &TransportBar::setPosition);
    connect(audioEngine_, &AudioEngine::durationChanged,
            transportBar_, &TransportBar::setDuration);
    connect(audioEngine_, &AudioEngine::stateChanged,
            transportBar_, &TransportBar::setState);
    connect(audioEngine_, &AudioEngine::audioFinished,
            this, &MainWindow::onPlaybackFinished);
    
    // Waveform seek
    connect(waveformWidget_, &WaveformWidget::seekRequested,
            audioEngine_, &AudioEngine::seek);
    connect(audioEngine_, &AudioEngine::positionChanged,
            waveformWidget_, &WaveformWidget::setPlayheadPosition);
    
    // Caption panel
    connect(captionPanel_, &CaptionPanel::seekRequested,
            audioEngine_, &AudioEngine::seek);
    connect(audioEngine_, &AudioEngine::positionChanged,
            captionPanel_, &CaptionPanel::setCurrentTime);
    connect(captionPanel_, &CaptionPanel::importRequested,
            this, &MainWindow::onImportCaptions);
    connect(captionPanel_, &CaptionPanel::exportRequested,
            this, &MainWindow::onExportCaptions);
    
    // Effects panel -> preview update
    connect(effectsPanel_, &EffectsPanel::effectsChanged,
            this, &MainWindow::updatePreview);
    connect(effectsPanel_, &EffectsPanel::stateChangeCompleted,
            this, &MainWindow::onEffectStateChanged);
    connect(effectsPanel_, &EffectsPanel::compareToggled,
            this, [this](bool) {
                onPreviewTimerTimeout();
            });
}

void MainWindow::applyTheme() {
    setStyleSheet(R"(
        QMainWindow {
            background-color: #1e1e1e;
        }
        QWidget {
            background-color: #1e1e1e;
            color: #e0e0e0;
        }
        QToolTip {
            background-color: #2d2d2d;
            color: #e0e0e0;
            border: 1px solid #3d3d3d;
            padding: 5px;
            border-radius: 4px;
        }
    )");
}

void MainWindow::onNewProject() {
    if (!confirmUnsavedChanges()) return;

    cancelPendingPreview();
    audioEngine_->stop();
    
    audioClip_.reset();
    originalSamples_.clear();
    audioEngine_->setAudioClip(nullptr);
    waveformWidget_->clear();
    effectsPanel_->clearEffects();
    captionPanel_->clearCaptions();
    commandHistory_->clear();
    
    currentFilePath_.clear();
    hasUnsavedChanges_ = false;
    loadedSampleRate_ = 44100;
    loadedChannels_ = 2;
    
    updateUIState();
    updateWindowTitle();
    
    statusBar()->showMessage("New project created", 3000);
}

void MainWindow::onOpenAudio() {
    if (!confirmUnsavedChanges()) return;
    
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Audio File",
        QString(),
        "Audio Files (*.mp3 *.wav);;MP3 Files (*.mp3);;WAV Files (*.wav);;All Files (*)"
    );
    
    if (!filePath.isEmpty()) {
        loadAudioFile(filePath);
    }
}

void MainWindow::loadAudioFile(const QString& filePath) {
    if (filePath.isEmpty()) {
        return;
    }
    
    logger_->log("Loading audio file: " + filePath.toStdString());
    
    cancelPendingPreview();
    audioEngine_->stop();
    
    statusBar()->showMessage("Loading " + filePath + "...");
    QApplication::processEvents();
    
    audioClip_ = std::make_shared<AudioClip>(filePath.toStdString(), logger_);
    
    if (!audioClip_->load()) {
        QMessageBox::critical(this, "Error", 
            "Failed to load audio file:\n" + filePath);
        audioClip_.reset();
        statusBar()->showMessage("Failed to load file", 3000);
        return;
    }
    
    loadedSampleRate_ = audioClip_->getSampleRate();
    loadedChannels_ = audioClip_->getChannels();
    
    originalSamples_ = audioClip_->getSamples();
    
    audioEngine_->setAudioClip(audioClip_);
    
    waveformWidget_->setSamples(audioClip_->getSamples(), loadedSampleRate_, loadedChannels_);
    
    effectsPanel_->clearEffects();
    captionPanel_->clearCaptions();
    commandHistory_->clear();
    
    currentFilePath_ = filePath;
    hasUnsavedChanges_ = false;
    
    updateUIState();
    updateWindowTitle();
    onAudioLoaded();
    
    statusBar()->showMessage("Loaded: " + filePath + 
        " (" + QString::number(loadedSampleRate_) + " Hz, " + 
        QString::number(loadedChannels_) + " ch)", 5000);
}

std::vector<float> MainWindow::getSamplesToSave() {
    auto effects = effectsPanel_->getEffectsForExport();
    
    if (effects.empty()) {
        if (logger_) {
            logger_->log("Saving original samples (no effects)");
        }
        return originalSamples_;
    }
    
    std::vector<float> result = originalSamples_;
    
    if (logger_) {
        logger_->log("Applying " + std::to_string(effects.size()) + " effects for save");
    }
    
    for (const auto& effect : effects) {
        if (!effect) continue;
        
        auto clonedEffect = effect->clone();
        clonedEffect->apply(result);
    }
    
    return result;
}

void MainWindow::onSaveAudio() {
    if (!audioClip_) return;
    
    QString savePath = currentFilePath_;
    
    if (!savePath.isEmpty()) {
        QFileInfo fileInfo(savePath);
        QString defaultName = fileInfo.baseName() + "_edited." + fileInfo.suffix();
        QString defaultPath = fileInfo.absolutePath() + "/" + defaultName;
        
        savePath = QFileDialog::getSaveFileName(this, "Save Audio File", defaultPath,
            "MP3 Files (*.mp3);;WAV Files (*.wav)");
    } else {
        savePath = QFileDialog::getSaveFileName(this, "Save Audio File", "output.mp3",
            "MP3 Files (*.mp3);;WAV Files (*.wav)");
    }

    if (savePath.isEmpty()) return;
    
    statusBar()->showMessage("Saving...");
    QApplication::processEvents();
    
    std::vector<float> samplesToSave = getSamplesToSave();
    
    if (logger_) {
        float maxSample = 0.0f;
        for (const float s : samplesToSave) {
            maxSample = std::max(maxSample, std::abs(s));
        }
        logger_->log("Saving " + std::to_string(samplesToSave.size()) + 
                     " samples, peak: " + std::to_string(maxSample));
    }
    
    std::vector<float> backup = audioClip_->getSamples();
    audioClip_->setSamples(std::move(samplesToSave));
    
    bool success = audioClip_->save(savePath.toStdString());
    
    audioClip_->setSamples(std::move(backup));
    
    if (success) {
        hasUnsavedChanges_ = false;
        updateWindowTitle();
        statusBar()->showMessage("Saved: " + savePath, 5000);
    } else {
        QMessageBox::critical(this, "Error", "Failed to save audio file.");
        statusBar()->showMessage("Save failed", 3000);
    }
}

void MainWindow::onExportAudio() {
    if (!audioClip_) return;
    
    QString filePath = QFileDialog::getSaveFileName(this, "Export Audio", 
        "exported_audio.mp3",
        "MP3 Files (*.mp3);;WAV Files (*.wav)");

    if (filePath.isEmpty()) return;
    
    statusBar()->showMessage("Exporting...");
    QApplication::processEvents();
    
    std::vector<float> samplesToSave = getSamplesToSave();
    std::vector<float> backup = audioClip_->getSamples();
    
    audioClip_->setSamples(std::move(samplesToSave));
    bool success = audioClip_->save(filePath.toStdString());
    audioClip_->setSamples(std::move(backup));
    
    if (success) {
        statusBar()->showMessage("Exported: " + filePath, 5000);
        QMessageBox::information(this, "Export Complete",
            "Audio exported successfully to:\n" + filePath);
    } else {
        QMessageBox::critical(this, "Error", "Failed to export audio file.");
        statusBar()->showMessage("Export failed", 3000);
    }
}

void MainWindow::onExit() {
    close();
}

void MainWindow::onUndo() {
    if (!commandHistory_->canUndo()) {
        statusBar()->showMessage("Nothing to undo", 1500);
        return;
    }
    
    commandHistory_->undo();
    
    if (audioClip_) {
        audioEngine_->setAudioClip(audioClip_);
        waveformWidget_->setSamples(audioClip_->getSamples(), loadedSampleRate_, loadedChannels_);
    }
    
    isPreviewMode_ = false;
    hasUnsavedChanges_ = commandHistory_->canUndo();
    
    updateUIState();
    updateWindowTitle();
    statusBar()->showMessage("Undo successful", 1500);
}

void MainWindow::onRedo() {
    if (!commandHistory_->canRedo()) {
        statusBar()->showMessage("Nothing to redo", 1500);
        return;
    }
    
    commandHistory_->redo();
    
    if (audioClip_) {
        audioEngine_->setAudioClip(audioClip_);
        waveformWidget_->setSamples(audioClip_->getSamples(), loadedSampleRate_, loadedChannels_);
    }
    
    isPreviewMode_ = false;
    hasUnsavedChanges_ = true;
    
    updateUIState();
    updateWindowTitle();
    statusBar()->showMessage("Redo successful", 1500);
}

void MainWindow::onImportCaptions() {
    QString filePath = QFileDialog::getOpenFileName(this, "Import Captions", QString(),
        "SubRip Files (*.srt);;All Files (*)");
    
    if (filePath.isEmpty()) return;
    
    std::vector<Caption> captions = captionParser_->parseSRT(filePath);
    
    if (captions.empty()) {
        QMessageBox::warning(this, "Import Failed",
            "No captions found in the file.\nPlease make sure it's a valid SRT file.");
        return;
    }
    
    captionPanel_->setCaptions(captions);
    exportCaptionsAction_->setEnabled(true);
    statusBar()->showMessage("Imported " + QString::number(captions.size()) + " captions", 3000);
}

void MainWindow::onExportCaptions() {
    std::vector<Caption> captions = captionPanel_->getCaptions();
    
    if (captions.empty()) {
        QMessageBox::warning(this, "No Captions", "There are no captions to export.");
        return;
    }
    
    QString filePath = QFileDialog::getSaveFileName(this, "Export Captions", "captions.txt",
        "Text Files (*.txt);;SubRip Files (*.srt)");
    
    if (filePath.isEmpty()) return;
    
    bool success = filePath.endsWith(".srt", Qt::CaseInsensitive) 
        ? captionParser_->exportSRT(filePath, captions)
        : captionParser_->exportTXT(filePath, captions);
    
    if (success) {
        statusBar()->showMessage("Captions exported: " + filePath, 3000);
    } else {
        QMessageBox::critical(this, "Export Failed", "Failed to export captions.");
    }
}

void MainWindow::onPlaybackFinished() {
    statusBar()->showMessage("Playback finished", 2000);
}

void MainWindow::onAudioLoaded() {
    statusBar()->showMessage("Ready", 2000);
}

void MainWindow::updateUIState() {
    bool hasAudio = (audioClip_ != nullptr);
    
    saveAction_->setEnabled(hasAudio);
    exportAction_->setEnabled(hasAudio);
    undoAction_->setEnabled(commandHistory_->canUndo());
    redoAction_->setEnabled(commandHistory_->canRedo());
    effectsPanel_->setEnabled(hasAudio);
}

void MainWindow::updateWindowTitle() {
    QString title = "Audio Editor";
    
    if (!currentFilePath_.isEmpty()) {
        QFileInfo fileInfo(currentFilePath_);
        title += " - " + fileInfo.fileName();
    }
    
    if (hasUnsavedChanges_) {
        title += " *";
    }
    
    setWindowTitle(title);
}

bool MainWindow::confirmUnsavedChanges() {
    if (!hasUnsavedChanges_) return true;
    
    return QMessageBox::question(this, "Unsaved Changes",
        "You have unsaved changes. Do you want to continue and discard them?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (confirmUnsavedChanges()) {
        cancelPendingPreview();
        audioEngine_->stop();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString filePath = urls.first().toLocalFile();
            if (filePath.endsWith(".mp3", Qt::CaseInsensitive) ||
                filePath.endsWith(".wav", Qt::CaseInsensitive) ||
                filePath.endsWith(".srt", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event) {
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    
    QString filePath = urls.first().toLocalFile();
    
    if (filePath.endsWith(".srt", Qt::CaseInsensitive)) {
        std::vector<Caption> captions = captionParser_->parseSRT(filePath);
        if (!captions.empty()) {
            captionPanel_->setCaptions(captions);
            exportCaptionsAction_->setEnabled(true);
            statusBar()->showMessage("Imported " + QString::number(captions.size()) + " captions", 3000);
        }
    } else if (confirmUnsavedChanges()) {
        loadAudioFile(filePath);
    }
}

void MainWindow::onRefreshAudioDevice() {
    if (audioClip_) {
        qint64 pos = audioEngine_->getPositionMs();
        bool wasPlaying = (audioEngine_->getState() == PlaybackState::Playing);
        
        audioEngine_->stop();
        audioEngine_->setAudioClip(audioClip_);
        audioEngine_->seek(pos);
        
        if (wasPlaying) audioEngine_->play();
        
        statusBar()->showMessage("Audio device refreshed", 2000);
    }
}

void MainWindow::updatePreview() {
    previewDebounceTimer_->start();
}

void MainWindow::onPreviewTimerTimeout() {
    if (!audioClip_ || audioClip_->getSamples().empty()) return;
    
    auto effects = effectsPanel_->getEffects();
    startPreviewComputation(effects);
}

void MainWindow::startPreviewComputation(const std::vector<std::shared_ptr<IEffect>>& effects) {
    if (!audioClip_) return;

    if (effects.empty()) {
        cancelPendingPreview();
        audioEngine_->revertToOriginal();
        waveformWidget_->setSamples(audioClip_->getSamples(), loadedSampleRate_, loadedChannels_);
        isPreviewMode_ = false;
        statusBar()->showMessage("Original audio", 1500);
        return;
    }

    // Queue if already computing
    if (previewWatcher_->isRunning()) {
        queuedEffects_ = effects;
        previewComputationQueued_ = true;
        return;
    }

    discardPreviewResult_.store(false);
    statusBar()->showMessage("Rendering preview...");

    auto baseSamples = audioClip_->getSamples();
    
    std::vector<std::shared_ptr<IEffect>> effectClones;
    effectClones.reserve(effects.size());
    for (const auto& effect : effects) {
        if (effect) {
            effectClones.push_back(effect->clone());
        }
    }

    auto future = QtConcurrent::run([baseSamples = std::move(baseSamples), 
                                      effectClones = std::move(effectClones)]() mutable {
        auto processed = baseSamples;
        
        for (const auto& effect : effectClones) {
            if (effect) {
                effect->apply(processed);
            }
        }

        return processed;
    });
    previewWatcher_->setFuture(future);
}

void MainWindow::onPreviewComputationFinished() {
    if (!previewWatcher_) return;

    if (discardPreviewResult_.load()) {
        discardPreviewResult_.store(false);
    } else {
        std::vector<float> processed = previewWatcher_->result();
        audioEngine_->previewWithSamples(processed);
        waveformWidget_->setSamples(processed, loadedSampleRate_, loadedChannels_);
        isPreviewMode_ = true;
        statusBar()->showMessage("Preview ready", 1000);
    }

    // Process queued computation if any
    if (previewComputationQueued_ && !queuedEffects_.empty()) {
        previewComputationQueued_ = false;
        auto pending = queuedEffects_;
        queuedEffects_.clear();
        startPreviewComputation(pending);
    } else {
        queuedEffects_.clear();
    }
}

void MainWindow::cancelPendingPreview() {
    if (!previewWatcher_) return;

    if (previewWatcher_->isRunning()) {
        discardPreviewResult_.store(true);
        previewWatcher_->waitForFinished();
    }

    previewComputationQueued_ = false;
    queuedEffects_.clear();
}

void MainWindow::onEffectStateChanged(const EffectsPanelState& oldState, 
                                       const EffectsPanelState& newState) {
    auto command = std::make_shared<EffectStateCommand>(
        effectsPanel_, oldState, newState, logger_);
    
    commandHistory_->executeCommand(command);
    
    updateUIState();
    logger_->log("Effect state change recorded for undo/redo");
}

void MainWindow::onApplyEffects() {
    if (!audioClip_) return;

    auto effects = effectsPanel_->getEffects();
    for (auto& effect : effects) {
        if (effect) {
            effect->apply(audioClip_->getSamplesRef());
        }
    }

    hasUnsavedChanges_ = true;
    updateWindowTitle();
    statusBar()->showMessage("Effects applied", 2000);
}

void MainWindow::updateCaptionSpeed() {
    float speedFactor = 1.0f;
    
    auto effects = effectsPanel_->getEffects();
    for (const auto& effect : effects) {
        if (auto* speed = dynamic_cast<SpeedChangeEffect*>(effect.get())) {
            speedFactor *= speed->getSpeedFactor();  // Multiply, don't just take first
        }
    }
    
    captionPanel_->setSpeedFactor(speedFactor);
}