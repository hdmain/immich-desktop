#pragma once

#include <QWidget>

class QComboBox;

namespace Aurora {

class ThemeManager;

class AppearancePage final : public QWidget {
    Q_OBJECT

public:
    explicit AppearancePage(ThemeManager *themeManager, QWidget *parent = nullptr);

private:
    void syncControls();

    ThemeManager *m_themeManager;
    QComboBox *m_themeCombo;
};

} // namespace Aurora
