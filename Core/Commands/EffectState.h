#pragma once

#include <QString>
#include <QMap>
#include <vector>

struct EffectState {
    QString effectType;
    bool enabled = true;
    QMap<QString, int> parameters;
};

struct EffectsPanelState {
    bool effectsEnabled = true;
    std::vector<EffectState> effects;
};
