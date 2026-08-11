#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QShowEvent;

namespace Aurora {

class ThemeManager;

class AppearancePage final : public QWidget {
    Q_OBJECT

public:
    explicit AppearancePage(ThemeManager *themeManager, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void syncControls();
    void refreshCacheSize();
    void saveCloseToTray(bool enabled);
    void saveAutoStart(bool enabled);
    void saveHoverPreview(bool enabled);

    ThemeManager *m_themeManager;
    QComboBox *m_themeCombo;
    QCheckBox *m_closeToTray;
    QCheckBox *m_autoStart;
    QCheckBox *m_hoverPreview = nullptr;
    QLabel *m_cacheSizeLabel;
};

} // namespace Aurora
