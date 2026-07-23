#pragma once

#include <QWidget>

class QCheckBox;
class QFrame;
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
    void refreshStarCard();
    void openGitHubStarPage();
    void dismissStarPrompt(bool permanently);
    static QString formatBytes(qint64 bytes);
    static QUrl githubRepoUrl();

    UpdateManager *m_updateManager;
    QLabel *m_versionLabel;
    QLabel *m_statusLabel;
    QProgressBar *m_progress;
    QCheckBox *m_autoCheck;
    QPushButton *m_refreshButton;
    QPushButton *m_updateButton;
    QFrame *m_starCard;
};

} // namespace Aurora
