#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QShowEvent;

namespace Aurora {

class ImmichClient;
class ThemeManager;

class AppearancePage final : public QWidget {
    Q_OBJECT

public:
    AppearancePage(ThemeManager *themeManager, ImmichClient *client, QWidget *parent = nullptr);

signals:
    void languageChangeRequested(const QString &language);
    void rememberViewedChanged(bool remember);
    void clearViewedRequested();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void syncControls();
    void refreshCacheSize();
    void startThumbnailPrefetch();
    void saveCloseToTray(bool enabled);
    void saveAutoStart(bool enabled);
    void saveHoverPreview(bool enabled);
    void saveCompactGrid(bool enabled);

    ThemeManager *m_themeManager;
    ImmichClient *m_client = nullptr;
    QComboBox *m_themeCombo;
    QComboBox *m_iconPackCombo = nullptr;
    QCheckBox *m_minimalistUi = nullptr;
    QLabel *m_iconPreview = nullptr;
    QCheckBox *m_closeToTray;
    QCheckBox *m_autoStart;
    QCheckBox *m_hoverPreview = nullptr;
    QCheckBox *m_compactGrid;
    QCheckBox *m_rememberViewed;
    QLabel *m_cacheSizeLabel;
    QLabel *m_prefetchStatus = nullptr;
    QPushButton *m_prefetchButton = nullptr;
};

} // namespace Aurora
