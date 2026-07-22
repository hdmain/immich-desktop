#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class QProgressBar;
class QPushButton;

namespace Aurora {

class UpdateManager;

class UpdatesPage final : public QWidget {
    Q_OBJECT

public:
    explicit UpdatesPage(UpdateManager *updateManager, QWidget *parent = nullptr);

private:
    void refreshUi();
    static QString formatBytes(qint64 bytes);

    UpdateManager *m_updateManager;
    QLabel *m_versionLabel;
    QLabel *m_statusLabel;
    QProgressBar *m_progress;
    QCheckBox *m_autoCheck;
    QPushButton *m_refreshButton;
    QPushButton *m_updateButton;
};

} // namespace Aurora
