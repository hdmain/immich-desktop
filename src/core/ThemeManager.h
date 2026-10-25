#pragma once

#include "core/AppSettings.h"

#include <QObject>

namespace Aurora {

enum class ColorRole {
    Background,
    Panel,
    Button,
    Accent,
    Text,
    MutedText,
    Border
};

class ThemeManager final : public QObject {
    Q_OBJECT

public:
    explicit ThemeManager(QObject *parent = nullptr);

    ThemePreset preset() const;
    const ThemePalette &palette() const;
    const ThemePalette &customPalette() const;
    QString iconPack() const { return m_settings.iconPack; }
    bool minimalistUi() const { return m_settings.minimalistUi; }

    void initialize();
    void setPreset(ThemePreset preset);
    void setIconPack(const QString &pack);
    void setMinimalistUi(bool enabled);
    void setCustomColor(ColorRole role, const QColor &color);
    void resetCustomPalette();

signals:
    void appearanceChanged();

private:
    void refresh(bool persist = true);
    QString buildStyleSheet() const;

    AppSettings m_store;
    AppearanceSettings m_settings;
    ThemePalette m_activePalette;
};

} // namespace Aurora
