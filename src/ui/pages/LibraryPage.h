#pragma once

#include "core/ImmichClient.h"

#include <QDate>
#include <QHash>
#include <QList>
#include <QWidget>

class QLabel;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QTimer;

namespace Aurora {

class MediaTile;

class LibraryPage final : public QWidget {
    Q_OBJECT

public:
    explicit LibraryPage(ImmichClient *client, QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void refresh();
    void loadMore();
    void showAssets(const QList<Aurora::ImmichAsset> &assets, const QString &nextPage);
    void showThumbnail(const QString &assetId, const QPixmap &thumbnail);
    void showThumbnailError(const QString &assetId, const QString &resultSize,
                            const QString &message);
    void showRequestError(const QString &operation, const QString &message);
    void openAsset(const Aurora::ImmichAsset &asset);

private:
    struct DaySection {
        QDate date;
        QLabel *header = nullptr;
        QList<MediaTile *> tiles;
    };

    void requestPage(int page, bool append);
    void clearTimeline();
    void layoutTimeline();
    void scheduleLayout();
    void updateEmptyState();
    QString formatDayHeader(const QDate &date) const;
    DaySection *sectionForDate(const QDate &date);

    ImmichClient *m_client;
    QScrollArea *m_scrollArea;
    QWidget *m_timelineHost;
    QLabel *m_status;
    QLabel *m_emptyState;
    QPushButton *m_refreshButton;
    QPushButton *m_loadMoreButton;
    QTimer *m_layoutTimer;
    QList<DaySection> m_sections;
    QHash<QString, MediaTile *> m_tilesById;
    QList<ImmichAsset> m_assets;
    QString m_nextPage;
    bool m_loading = false;
    bool m_appendRequest = false;
};

} // namespace Aurora
