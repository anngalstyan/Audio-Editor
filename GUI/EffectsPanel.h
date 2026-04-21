#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <vector>
#include <memory>
#include "../Core/Commands/EffectState.h"

class EffectWidget;
class IEffect;
class ILogger;

class EffectsPanel : public QWidget {
    Q_OBJECT

public:
    explicit EffectsPanel(std::shared_ptr<ILogger> logger, QWidget* parent = nullptr);
    ~EffectsPanel() override = default;
    
    // Non-copyable
    EffectsPanel(const EffectsPanel&) = delete;
    EffectsPanel& operator=(const EffectsPanel&) = delete;
     std::vector<std::shared_ptr<IEffect>> getEffects() const;

     std::vector<std::shared_ptr<IEffect>> getEffectsForExport() const;

     bool areEffectsEnabled() const noexcept { return effectsEnabled_; }
     size_t effectCount() const noexcept { return effectWidgets_.size(); }

    void clearEffects();

    void setEnabled(bool enabled);

    void restoreState(const EffectsPanelState& state);

     EffectsPanelState saveState() const;

signals:

    void effectsChanged();

    void compareToggled(bool effectsEnabled);

    void applyEffectsRequested();

    void stateChangeCompleted(const EffectsPanelState& oldState, 
                              const EffectsPanelState& newState);

private slots:
    void onAddEffectClicked();
    void onApplyClicked();
    void onEffectRemoved(EffectWidget* widget);
    void onEffectParameterChanged();
    void onCompareToggled(bool enabled);

private:
    void setupUI();

    void addEffectWidgetInternal(const QString& effectType, 
                                  const QMap<QString, int>& params = {},
                                  bool emitSignals = false);

    std::shared_ptr<ILogger> logger_;
    
    // UI Components
    QVBoxLayout* mainLayout_;
    QComboBox* effectTypeCombo_;
    QPushButton* addButton_;
    QPushButton* compareButton_;
    QScrollArea* scrollArea_;
    QWidget* effectsContainer_;
    QVBoxLayout* effectsLayout_;
    QLabel* noEffectsLabel_;
    
    // Effect widgets
    std::vector<EffectWidget*> effectWidgets_;
    
    // State
    bool effectsEnabled_;
    EffectsPanelState lastSavedState_;
    bool isRestoringState_;
};