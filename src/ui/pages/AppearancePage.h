#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;

namespace Aurora {

class ThemeManager;

class AppearancePage final : public QWidget {
    Q_OBJECT

public:
    explicit AppearancePage(ThemeManager *themeManager, QWidget *parent = nullptr);

private:
    void syncControls();
    void saveCloseToTray(bool enabled);

    ThemeManager *m_themeManager;
    QComboBox *m_themeCombo;
    QCheckBox *m_closeToTray;
};

} // namespace Aurora
