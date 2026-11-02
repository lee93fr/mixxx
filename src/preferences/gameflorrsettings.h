#pragma once

#include <QDir>
#include <QString>

inline QString gameflorrStatePath() {
    const QString overridePath = qEnvironmentVariable("GAMEFLORR_MIXXX_STATE_PATH");
    return overridePath.isEmpty()
            ? QDir::home().filePath(QStringLiteral(".gameflorr/mixxx-state.json"))
            : overridePath;
}
