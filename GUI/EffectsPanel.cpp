#include "EffectsPanel.h"
#include "EffectWidget.h"
#include "../Core/Effects/Reverb.h"
#include "../Core/Effects/Speed.h"
#include "../Core/Effects/Volume.h"
#include <QHBoxLayout>
#include <algorithm>
#include "../Core/EffectFactory.h"

EffectsPanel::EffectsPanel(std::shared_ptr<ILogger> logger, QWidget* parent)
    : QWidget(parent)
    , logger_(std::move(logger))
    , effectsEnabled_(true)
    , isRestoringState_(false)
{
    setupUI();
    lastSavedState_ = saveState();
}

void EffectsPanel::setupUI() {
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(10, 10, 10, 10);
    mainLayout_->setSpacing(10);

    QLabel* headerLabel = new QLabel("Effects", this);
    headerLabel->setStyleSheet(R"(
        QLabel {
            color: #00bcd4;
            font-size: 16px;
            font-weight: bold;
            padding: 5px 0;
        }
    )");
    mainLayout_->addWidget(headerLabel);

    QHBoxLayout* addLayout = new QHBoxLayout();
    effectTypeCombo_ = new QComboBox(this);
    effectTypeCombo_->addItems({"Reverb", "Speed", "Volume"});
    effectTypeCombo_->setStyleSheet(R"(
        QComboBox {
            background-color: #3d3d3d;
            color: #e0e0e0;
            border: 1px solid #555555;
            border-radius: 4px;
            padding: 5px 10px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox QAbstractItemView {
            background-color: #3d3d3d;
            color: #e0e0e0;
            selection-background-color: #00bcd4;
        }
    )");
    
    addButton_ = new QPushButton("Add", this);
    addButton_->setStyleSheet(R"(
        QPushButton {
            background-color: #4caf50;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 6px 15px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #66bb6a;
        }
        QPushButton:pressed {
            background-color: #388e3c;
        }
        QPushButton:disabled {
            background-color: #2a2a2a;
            color: #555555;
        }
    )");
    connect(addButton_, &QPushButton::clicked, this, &EffectsPanel::onAddEffectClicked);
    
    addLayout->addWidget(effectTypeCombo_, 1);
    addLayout->addWidget(addButton_);
    mainLayout_->addLayout(addLayout);

    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setStyleSheet(R"(
        QScrollArea {
            border: none;
            background-color: transparent;
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
    )");
    
    effectsContainer_ = new QWidget();
    effectsContainer_->setStyleSheet("background-color: transparent;");
    effectsLayout_ = new QVBoxLayout(effectsContainer_);
    effectsLayout_->setContentsMargins(0, 0, 0, 0);
    effectsLayout_->setSpacing(8);
    effectsLayout_->addStretch();
    
    scrollArea_->setWidget(effectsContainer_);
    mainLayout_->addWidget(scrollArea_, 1);

    noEffectsLabel_ = new QLabel("No effects added.\nUse the dropdown above to add effects.", this);
    noEffectsLabel_->setAlignment(Qt::AlignCenter);
    noEffectsLabel_->setStyleSheet("color: #808080; font-size: 12px; padding: 20px;");
    mainLayout_->addWidget(noEffectsLabel_);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    compareButton_ = new QPushButton("Compare On/Off", this);
    compareButton_->setCheckable(true);
    compareButton_->setChecked(true);
    compareButton_->setToolTip("Toggle effects on/off to compare with original");
    compareButton_->setStyleSheet(R"(
        QPushButton {
            background-color: #3d3d3d;
            color: #e0e0e0;
            border: none;
            border-radius: 4px;
            padding: 8px 12px;
        }
        QPushButton:hover {
            background-color: #4d4d4d;
        }
        QPushButton:checked {
            background-color: #00bcd4;
            color: #000000;
        }
    )");
    connect(compareButton_, &QPushButton::toggled, this, &EffectsPanel::onCompareToggled);
    
    buttonLayout->addWidget(compareButton_);
    buttonLayout->addStretch();
    mainLayout_->addLayout(buttonLayout);
    
    setStyleSheet(R"(
        EffectsPanel {
            background-color: #252525;
        }
    )");
}

void EffectsPanel::onAddEffectClicked() {
    QString type = effectTypeCombo_->currentText();
    
    EffectsPanelState oldState = saveState();
    
    addEffectWidgetInternal(type, {}, false);
    
    EffectsPanelState newState = saveState();
    emit stateChangeCompleted(oldState, newState);
    lastSavedState_ = newState;
    
    emit effectsChanged();
}

void EffectsPanel::addEffectWidgetInternal(const QString& effectType, 
                                            const QMap<QString, int>& params,
                                            bool emitSignals) {
    if (effectType.isEmpty()) {
        return;
    }
    
    EffectsPanelState oldState;
    if (emitSignals) {
        oldState = saveState();
    }
    
    EffectWidget* widget = new EffectWidget(effectType, logger_, this);
    
    if (!params.isEmpty()) {
        widget->setParameterState(params);
    }
    
    connect(widget, &EffectWidget::removeRequested, this, &EffectsPanel::onEffectRemoved);
    connect(widget, &EffectWidget::parameterChanged, this, &EffectsPanel::onEffectParameterChanged);
    
    effectsLayout_->insertWidget(effectsLayout_->count() - 1, widget);
    effectWidgets_.push_back(widget);
    
    noEffectsLabel_->setVisible(false);
    scrollArea_->setVisible(true);
    
    if (emitSignals) {
        EffectsPanelState newState = saveState();
        emit stateChangeCompleted(oldState, newState);
        lastSavedState_ = newState;
        emit effectsChanged();
    }
}

void EffectsPanel::onEffectRemoved(EffectWidget* widget) {
    if (!widget) {
        return;
    }
    
    EffectsPanelState oldState = saveState();
    
    auto it = std::find(effectWidgets_.begin(), effectWidgets_.end(), widget);
    if (it != effectWidgets_.end()) {
        effectWidgets_.erase(it);
    }
    widget->deleteLater();
    
    if (effectWidgets_.empty()) {
        noEffectsLabel_->setVisible(true);
    }
    
    if (!isRestoringState_) {
        EffectsPanelState newState = saveState();
        emit stateChangeCompleted(oldState, newState);
        lastSavedState_ = newState;
    }
    
    emit effectsChanged();
}

void EffectsPanel::onEffectParameterChanged() {
    if (isRestoringState_) {
        emit effectsChanged();
        return;
    }
    
    EffectsPanelState currentState = saveState();
    
    // Check if state actually changed
    bool stateChanged = false;
    if (currentState.effects.size() != lastSavedState_.effects.size()) {
        stateChanged = true;
    } else {
        for (size_t i = 0; i < currentState.effects.size(); ++i) {
            if (currentState.effects[i].parameters != lastSavedState_.effects[i].parameters ||
                currentState.effects[i].enabled != lastSavedState_.effects[i].enabled) {
                stateChanged = true;
                break;
            }
        }
    }
    
    if (stateChanged) {
        emit stateChangeCompleted(lastSavedState_, currentState);
        lastSavedState_ = currentState;
    }
    
    emit effectsChanged();
}

void EffectsPanel::onCompareToggled(bool enabled) {
    effectsEnabled_ = enabled;
    emit compareToggled(enabled);
    emit effectsChanged();
}

std::vector<std::shared_ptr<IEffect>> EffectsPanel::getEffects() const {
    if (!effectsEnabled_) {
        return {};
    }
    
    std::vector<std::shared_ptr<IEffect>> effects;
    effects.reserve(effectWidgets_.size());
    
    for (const auto& widget : effectWidgets_) {
        if (!widget) continue;
        
        auto effect = widget->createEffect();
        if (effect) {
            effects.push_back(effect);
        }
    }
    
    std::sort(effects.begin(), effects.end(), 
        [](const std::shared_ptr<IEffect>& a, const std::shared_ptr<IEffect>& b) {
            auto priority = [](const std::shared_ptr<IEffect>& e) -> int {
                if (!e) return 99;
                if (e->getName() == "Speed") return 0;
                if (e->getName() == "Volume") return 1;
                if (e->getName() == "Reverb") return 2;
                return 1;
            };
            return priority(a) < priority(b);
        });
    
    return effects;
}

std::vector<std::shared_ptr<IEffect>> EffectsPanel::getEffectsForExport() const {
    std::vector<std::shared_ptr<IEffect>> effects;
    effects.reserve(effectWidgets_.size());
    
    for (const auto& widget : effectWidgets_) {
        if (!widget) continue;
        
        auto effect = widget->createEffect();
        if (effect) {
            effects.push_back(effect);
        }
    }
    
    // Sort effects by processing order
    std::sort(effects.begin(), effects.end(), 
        [](const std::shared_ptr<IEffect>& a, const std::shared_ptr<IEffect>& b) {
            auto priority = [](const std::shared_ptr<IEffect>& e) -> int {
                if (!e) return 99;
                if (e->getName() == "Speed") return 0;
                if (e->getName() == "Volume") return 1;
                if (e->getName() == "Reverb") return 2;
                return 1;
            };
            return priority(a) < priority(b);
        });
    
    return effects;
}

EffectsPanelState EffectsPanel::saveState() const {
    EffectsPanelState state;
    state.effectsEnabled = effectsEnabled_;
    state.effects.reserve(effectWidgets_.size());
    
    for (const auto& widget : effectWidgets_) {
        if (!widget) continue;
        
        EffectState effectState;
        effectState.effectType = widget->getEffectType();
        effectState.enabled = widget->isEffectEnabled();
        effectState.parameters = widget->getParameterState();
        state.effects.push_back(effectState);
    }
    
    return state;
}

void EffectsPanel::restoreState(const EffectsPanelState& state) {
    isRestoringState_ = true;
    
    // Remove all existing widgets
    while (!effectWidgets_.empty()) {
        auto widget = effectWidgets_.back();
        effectWidgets_.pop_back();
        effectsLayout_->removeWidget(widget);
        widget->deleteLater();
    }
    
    // Recreate widgets from state
    for (const auto& effectState : state.effects) {
        addEffectWidgetInternal(effectState.effectType, effectState.parameters, false);
    }
    
    noEffectsLabel_->setVisible(effectWidgets_.empty());
    
    // Restore compare button state
    compareButton_->blockSignals(true);
    compareButton_->setChecked(state.effectsEnabled);
    compareButton_->blockSignals(false);
    effectsEnabled_ = state.effectsEnabled;
    
    lastSavedState_ = state;
    
    isRestoringState_ = false;
    
    emit effectsChanged();
}

void EffectsPanel::clearEffects() {
    isRestoringState_ = true;
    
    for (auto widget : effectWidgets_) {
        if (widget) {
            widget->deleteLater();
        }
    }
    effectWidgets_.clear();
    noEffectsLabel_->setVisible(true);
    
    lastSavedState_ = saveState();
    isRestoringState_ = false;
}

void EffectsPanel::setEnabled(bool enabled) {
    addButton_->setEnabled(enabled);
    compareButton_->setEnabled(enabled);
    effectTypeCombo_->setEnabled(enabled);
    for (auto widget : effectWidgets_) {
        if (widget) {
            widget->setEnabled(enabled);
        }
    }
}

void EffectsPanel::onApplyClicked() {
    emit effectsChanged();
}