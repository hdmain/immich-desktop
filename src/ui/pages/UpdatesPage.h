#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QTextEdit;

namespace Aurora {

class UpdateManager;
struct UpdateInfo;

class UpdatesPage final : public QWidget {
    Q_OBJECT

public:
    explicit UpdatesPage(UpdateManager *updateManager, QWidget *parent = nullptr);

private:
    void refreshUi();
    void onUpdateAvailable(const UpdateInfo &info);
    static QString formatBytes(qint64 bytes);
    static QString packageLabel(const UpdateInfo &info);

    UpdateManager *m_updateManager;
    QLabel *m_statusLabel;
    QLabel *m_detailLabel;
    QLabel *m_packageLabel;
    QTextEdit *m_notesView;
    QProgressBar *m_progress;
    QCheckBox *m_autoCheck;
    QPushButton *m_checkButton;
    QPushButton *m_downloadButton;
    QPushButton *m_installButton;
    QPushButton *m_skipButton;
};

} // namespace Aurora
