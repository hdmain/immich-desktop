#pragma once

#include "core/ImmichClient.h"

#include <QDate>
#include <QFrame>
#include <QHash>
#include <QList>
#include <QMultiHash>
#include <QPixmap>
#include <QPointer>
#include <QSet>
#include <QWidget>

class QEvent;
class QHideEvent;
class QKeyEvent;
class QLabel;
class QMouseEvent;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QShowEvent;
class QStackedWidget;
class QTimer;
class QVBoxLayout;

namespace Aurora {

class MediaTile;
class VideoHoverPreview;
class ZoomPanWidget;

class ExploreCard final : public QFrame {
    Q_OBJECT

public:
    enum class Style { Person, Place, Media };

    explicit ExploreCard(Style style, QWidget *parent = nullptr);
    void setCaption(const QString &text);
    void setPixmap(const QPixmap &pixmap);

signals:
    void activated();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    Style m_style;
    QLabel *m_image;
    QLabel *m_caption;
};

class ExplorePage final : public QWidget {
    Q_OBJECT

public:
    explicit ExplorePage(ImmichClient *client, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void refresh();
    void showExplore(const Aurora::ImmichExploreData &data, bool fromCache);
    void showFilteredAssets(const QString &filterKind, const QString &filterValue,
                            const QList<Aurora::ImmichAsset> &assets, const QString &nextPage);
    void showPersonThumbnail(const QString &personId, const QPixmap &thumbnail);
    void showAssetThumbnail(const QString &assetId, const QPixmap &thumbnail);
    void showPreviewImage(const QString &assetId, const QPixmap &preview);
    void showRequestError(const QString &operation, const QString &message);
    void openPerson(const Aurora::ImmichPerson &person);
    void openPlace(const Aurora::ImmichPlace &place);
    void openAsset(const Aurora::ImmichAsset &asset);
    void backToBrowse();
    void backToCollection();
    void requestCollectionNextPage();
    void layoutCollection();
    void updateVisibleCollectionThumbs();

private:
    enum class Page : int { Browse = 0, Collection = 1, Preview = 2 };

    struct SectionRow {
        QLabel *header = nullptr;
        QScrollArea *scroll = nullptr;
        QWidget *host = nullptr;
        QList<ExploreCard *> cards;
    };

    struct DaySection {
        QDate date;
        QLabel *header = nullptr;
        QList<MediaTile *> tiles;
    };

    void clearSections();
    void updateEmptyState();
    void populateSection(SectionRow *section, bool visible);
    void showCollectionPage(const QString &title, const QString &subtitle,
                            const QString &filterKind, const QString &filterValue,
                            const QPixmap &heroThumb, bool personStyle);
    void clearCollectionTimeline();
    void appendCollectionAssets(const QList<ImmichAsset> &assets);
    void updateCollectionLoadMore(const QString &nextPage);
    void showPreviewPage(const ImmichAsset &asset);
    void setPage(Page page);
    void setCollectionHero(const QPixmap &thumb, bool personStyle);
    void scheduleCollectionLayout();
    void scheduleCollectionVisibility();
    void releaseUiPixmaps();
    DaySection *sectionForDate(const QDate &date);
    QString formatDayHeader(const QDate &date) const;

    ImmichClient *m_client;
    QStackedWidget *m_stack = nullptr;

    // Browse
    QWidget *m_browsePage = nullptr;
    QScrollArea *m_browseScroll = nullptr;
    QWidget *m_browseContent = nullptr;
    QVBoxLayout *m_browseLayout = nullptr;
    QLabel *m_browseStatus = nullptr;
    QLabel *m_emptyState = nullptr;
    QPushButton *m_refreshButton = nullptr;
    SectionRow m_peopleSection;
    SectionRow m_placesSection;
    SectionRow m_recentSection;
    QHash<QString, ExploreCard *> m_personCards;
    QMultiHash<QString, ExploreCard *> m_browseAssetCards;
    QHash<QString, QPixmap> m_personThumbCache;

    // Collection detail (Library-style timeline for one person/place)
    QWidget *m_collectionPage = nullptr;
    QPushButton *m_collectionBack = nullptr;
    QLabel *m_collectionHero = nullptr;
    QLabel *m_collectionTitle = nullptr;
    QLabel *m_collectionSubtitle = nullptr;
    QScrollArea *m_collectionScroll = nullptr;
    QWidget *m_collectionHost = nullptr;
    QPushButton *m_collectionLoadMore = nullptr;
    VideoHoverPreview *m_collectionHoverPreview = nullptr;
    QTimer *m_collectionLayoutTimer = nullptr;
    QTimer *m_collectionVisibilityTimer = nullptr;
    QList<DaySection> m_collectionSections;
    QHash<QString, QPointer<MediaTile>> m_collectionTiles;
    QSet<QString> m_collectionRequestedThumbs;
    QString m_collectionFilterKind;
    QString m_collectionFilterValue;
    QString m_collectionHeroAssetId;
    QString m_collectionNextPage;
    QString m_pendingCollectionTitle;
    bool m_pendingPersonStyle = false;
    bool m_collectionLoadingMore = false;
    bool m_collectionPersonStyle = false;
    bool m_collectionCompactGrid = false;

    // Preview
    QWidget *m_previewPage = nullptr;
    QPushButton *m_previewBack = nullptr;
    QLabel *m_previewTitle = nullptr;
    ZoomPanWidget *m_previewView = nullptr;
    QString m_previewAssetId;
    bool m_previewFromCollection = false;

    bool m_loadedOnce = false;
    bool m_loading = false;
};

} // namespace Aurora
