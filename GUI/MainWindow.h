#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QTimer>
#include <QFileInfo>
#include <QFutureWatcher>
#include <memory>
#include <atomic>
#include "EffectsPanel.h"

class AudioEngine;
class AudioClip;
class TransportBar;
class WaveformWidget;
class CaptionPanel;
class CaptionParser;
class ILogger;
class CommandHistory;
class IEffect;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:

    void onNewProject();
    void onOpenAudio();
    void onSaveAudio();
    void onExportAudio();
    void onExit();

    void onUndo();
    void onRedo();

    void onTogglePlayPause();

    void onImportCaptions();
    void onExportCaptions();

    void onPlaybackFinished();
    void onRefreshAudioDevice();
    void onAudioLoaded();

    void updateWindowTitle();

    void onApplyEffects();
    void onPreviewTimerTimeout();
    void onPreviewComputationFinished();
    void onEffectStateChanged(const EffectsPanelState& oldState, 
                              const EffectsPanelState& newState);

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupConnections();
    void setupShortcuts();
    void applyTheme();
    void loadAudioFile(const QString& filePath);
    void updateUIState();
    bool confirmUnsavedChanges();
    void updateCaptionSpeed();
    void updatePreview();
    void startPreviewComputation(const std::vector<std::shared_ptr<IEffect>>& effects);
    void cancelPendingPreview();
     std::vector<float> getSamplesToSave();

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<AudioClip> audioClip_;
    AudioEngine* audioEngine_;

    std::unique_ptr<CommandHistory> commandHistory_;
    std::unique_ptr<CaptionParser> captionParser_;

    std::vector<float> originalSamples_;

    int loadedSampleRate_ = 44100;
    int loadedChannels_ = 2;

    QWidget* centralWidget_;
    TransportBar* transportBar_;
    WaveformWidget* waveformWidget_;
    EffectsPanel* effectsPanel_;
    CaptionPanel* captionPanel_;

    QMenu* fileMenu_;
    QMenu* editMenu_;
    QMenu* effectsMenu_;
    QMenu* helpMenu_;

    QAction* newAction_;
    QAction* openAction_;
    QAction* saveAction_;
    QAction* exportAction_;
    QAction* exitAction_;
    QAction* undoAction_;
    QAction* redoAction_;
    QAction* importCaptionsAction_;
    QAction* exportCaptionsAction_;
    QAction* aboutAction_;

    QTimer* previewDebounceTimer_;
    QFutureWatcher<std::vector<float>>* previewWatcher_;
    bool previewComputationQueued_;
    std::vector<std::shared_ptr<IEffect>> queuedEffects_;
    std::atomic<bool> discardPreviewResult_;

    QString currentFilePath_;
    bool hasUnsavedChanges_;
    bool isPreviewMode_;
};

#endif