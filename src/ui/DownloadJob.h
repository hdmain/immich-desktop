#pragma once

#include "core/ImmichTypes.h"

#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>

class QWidget;

namespace Aurora {

class ImmichClient;

// Downloads originals one after another, with a progress toast that has Cancel.
// Single downloads go to the path the user picked; a selection goes into a
// folder, asking what to do when a file name is already taken.
class DownloadJob final : public QObject {
    Q_OBJECT

public:
    DownloadJob(ImmichClient *client, QWidget *window, QObject *parent = nullptr);

    // Downloads asked for while a job runs join its queue.
    void downloadTo(const ImmichAsset &asset, const QString &path);
    void downloadInto(const QList<ImmichAsset> &assets, const QString &folder);
    bool isRunning() const { return m_current >= 0; }

private:
    struct Item {
        ImmichAsset asset;
        QString path;   // Set for a single download.
        QString folder; // Set for a selection; the file name is decided when it's due.
        bool lookedUp = false;
    };
    enum class ClashPolicy { Ask, Rename, Skip };

    void start(const QList<Item> &items);
    void next();
    void download(Item &item);
    QString wantedPath(const Item &item) const;
    bool isTaken(const QString &path) const;
    QString resolvePath(const Item &item, bool *skip);
    QString clashDetails(const QString &path, qint64 incomingSize) const;
    void handleProgress(const QString &assetId, qint64 received, qint64 total);
    void handleFinished(const QString &assetId, bool ok, const QString &error);
    void cancel();
    void finish();
    QString progressText(int percent) const;

    ImmichClient *m_client;
    QPointer<QWidget> m_window;
    QList<Item> m_items;
    int m_current = -1;
    QString m_currentId;
    QString m_currentPath;
    QString m_waitingForInfo; // Asset id whose file name is being looked up.
    QSet<QString> m_usedPaths; // Paths chosen in this job, so two photos don't collide.
    ClashPolicy m_clashPolicy = ClashPolicy::Ask;
    bool m_cancelled = false;
    int m_saved = 0;
    int m_skipped = 0;
    int m_failed = 0;
    QString m_lastError;
};

} // namespace Aurora
