#pragma once

#include "core/ImmichClient.h"
#include "ui/BrowseSection.h"

#include <QDate>
#include <QFrame>
#include <QHash>
#include <QList>
#include <QMultiHash>
#include <QPixmap>
#include <QPointer>
#include <QSet>
#include <QWidget>

#include <functional>

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

class ThemeManager;
class MediaTile;
class MediaViewerDialog;
class VideoHoverPreview;
class SortButton;
class TileImageScheduler;
class ZoomSlider;

class ExploreCard final : public QFrame {
    Q_OBJECT

public:
    enum class Style { Person, Place, Media };

    explicit ExploreCard(Style style, QWidget *parent = nullptr);
    void setCaption(const QString &text);
    void setPixmap(const QPixmap &pixmap);
    // Scales person and place cards with the grid zoom (index pages).
    void setTileSize(int size);
    QSize imageSize() const;

signals:
    void activated();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void showPixmap();

    Style m_style;
    QLabel *m_image;
    QPixmap m_source;
    QLabel *m_caption;
};

class ExplorePage final : public QWidget {
    Q_OBJECT

public:
    ExplorePage(ThemeManager *themeManager, ImmichClient *client, QWidget *parent = nullptr);

signals:
    void copyRequested(const Aurora::ImmichAsset &asset);
    void downloadRequested(const Aurora::ImmichAsset &asset);
    void trashRequested(const Aurora::ImmichAsset &asset);
    void deleteRequested(const Aurora::ImmichAsset &asset);
    void personRequested(const Aurora::ImmichPerson &person);
    void placeRequested(const QString &city);
    void dateRequested(const QDate &date);
    void albumRequested(const Aurora::ImmichAlbum &album);
    void tagRequested(const Aurora::ImmichTag &tag);
    void folderRequested(const QString &path, const QString &title);
    // Show everything recently added: the Library, sorted by date added.
    void recentlyAddedRequested();

    void sectionShown(Aurora::BrowseSection section, const QString &itemValue,
                      const QString &title);
    void overviewShown();
    void listRefreshRequested(Aurora::BrowseSection section);

public slots:
    void showOverview();
    void showSection(Aurora::BrowseSection section);
    void showPerson(const Aurora::ImmichPerson &person);
    void showPlace(const QString &city);
    void showAlbum(const Aurora::ImmichAlbum &album);
    void showTag(const Aurora::ImmichTag &tag);
    void showFolder(const QString &path, const QString &title);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void refresh();
    void showExplore(const Aurora::ImmichExploreData &data, bool fromCache);
    void showFilteredAssets(const QString &filterKind, const QString &filterValue,
                            const QList<Aurora::ImmichAsset> &assets, const QString &nextPage,
                            const Aurora::AssetSort &sort);
    void showPersonThumbnail(const QString &personId, const QPixmap &thumbnail);
    void showAssetThumbnail(const QString &assetId, const QPixmap &thumbnail);
    void showRequestError(const QString &operation, const QString &message);
    void handleAssetsDeleted(const QStringList &assetIds);
    void removeCollectionTiles(const QStringList &assetIds);
    void openPerson(const Aurora::ImmichPerson &person);
    void openPlace(const Aurora::ImmichPlace &place);
    void backToBrowse();
    void requestCollectionNextPage();
    void maybeLoadMoreCollection();
    void layoutCollection();
    void updateVisibleCollectionThumbs();

private:
    enum class Page : int { Browse = 0, Collection = 1, Index = 2 };

    struct Crumb {
        QString label;
        std::function<void()> open;
    };

    struct SectionRow {
        QWidget *header = nullptr; // Title row, with room for a link like View all.
        QScrollArea *scroll = nullptr;
        QWidget *host = nullptr;
        QList<ExploreCard *> cards;
    };

    struct DaySection {
        QDate date;
        QLabel *header = nullptr;
        QList<MediaTile *> tiles;
    };

    void showTagTrail(const ImmichTag &tag);
    void showFolderTrail(const QString &path, const QString &title);
    void showIndex(BrowseSection section);
    void showLibraryView(const QString &kind, BrowseSection section, const QString &title);
    void setCrumbs(const QList<Crumb> &crumbs, const QString &title);
    void setChips(const QList<Crumb> &chips, const QString &iconResource);
    QList<Crumb> folderChildren(const QString &path);
    // The open collection's sort, limited to what it offers.
    AssetSort effectiveCollectionSort() const;
    QList<AssetSort::Field> collectionSortFields() const;
    QString collectionSortKey() const;
    void loadCollectionSort();
    void chooseCollectionSort(const AssetSort &sort);
    void shuffleCollection();
    void showRandomAssets(const QString &kind, const QString &value,
                          const QList<Aurora::ImmichAsset> &assets, bool more);
    void reloadCollection();
    void openCollection(const QString &kind, const QString &value, const QString &title,
                        BrowseSection section);
    void clearSections();
    void updateEmptyState();
    void populateSection(SectionRow *section, bool visible);
    void showCollectionPage(const QString &title, const QString &subtitle,
                            const QString &filterKind, const QString &filterValue,
                            const QPixmap &heroThumb, bool personStyle);
    void clearCollectionTimeline();
    void appendCollectionAssets(const QList<ImmichAsset> &assets);
    void updateCollectionLoadMore(const QString &nextPage);
    // Where the viewer was opened from, so it steps through the same list.
    enum class AssetSource { Collection, RecentlyAdded, RecentlyViewed };
    ImmichAsset adjacentAsset(AssetSource source, const QString &assetId, int step);
    void setPage(Page page);
    MediaViewerDialog *openAsset(const Aurora::ImmichAsset &asset, AssetSource source);
    void populateViewedRow();
    void showRecentlyViewed();
    void setCollectionTileSize(int size);
    void applyIndexZoom();
    void clearTrashView();
    void updateCollectionSubtitle();
    void setCollectionHero(const QPixmap &thumb, bool personStyle);
    void scheduleCollectionLayout();
    void scheduleCollectionVisibility();
    void releaseUiPixmaps();
    DaySection *sectionForDate(const QDate &date);
    QString formatDayHeader(const QDate &date) const;

    ThemeManager *m_themeManager;
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
    SectionRow m_viewedSection;
    QList<ImmichAsset> m_viewedAssets;
    QHash<QString, ExploreCard *> m_personCards;
    QMultiHash<QString, ExploreCard *> m_browseAssetCards;
    QList<ImmichAsset> m_recentAssets;
    QHash<QString, QPixmap> m_personThumbCache;

    // Lists behind the index pages, breadcrumbs and child chips.
    QList<ImmichAlbum> m_albums;
    QList<ImmichTag> m_tags;
    QStringList m_folderPaths;
    QString m_folderRoot;
    QList<ImmichPerson> m_people;
    QList<ImmichPerson> m_allPeople; // Includes hidden people.
    QSet<BrowseSection> m_loadedLists;

    // Index (all albums, tags, folders or people)
    QWidget *m_indexPage = nullptr;
    QLabel *m_indexTitle = nullptr;
    QLabel *m_indexSubtitle = nullptr;
    QWidget *m_indexHost = nullptr;
    QScrollArea *m_indexScroll = nullptr;
    ZoomSlider *m_indexZoomSlider = nullptr;
    QHash<QString, int> m_indexRequestedCovers;
    QPushButton *m_showHiddenPeople = nullptr;
    QMultiHash<QString, ExploreCard *> m_indexAssetCards;
    QHash<QString, ExploreCard *> m_indexPersonCards;
    BrowseSection m_indexSection = BrowseSection::Albums;

    // Collection detail (Library-style timeline for one person, place, album, ...)
    QWidget *m_collectionPage = nullptr;
    QWidget *m_collectionTrail = nullptr;
    QWidget *m_collectionChips = nullptr;
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
    QSet<QString> m_favoriteRequests; // Sent from tile menus, awaiting a reply.
    TileImageScheduler *m_collectionTileImages = nullptr;
    QString m_collectionFilterKind;
    QString m_collectionFilterValue;
    QString m_collectionHeroAssetId;
    QString m_collectionNextPage;
    QString m_pendingCollectionTitle;
    bool m_pendingPersonStyle = false;
    bool m_collectionLoadingMore = false;
    bool m_collectionLoadFailed = false; // Stops loading on scroll until "Try again".
    bool m_collectionPersonStyle = false;
    bool m_collectionCompactGrid = false;
    int m_collectionTileSize = 180;
    ZoomSlider *m_collectionZoomSlider = nullptr;
    SortButton *m_collectionSortButton = nullptr;
    QPushButton *m_collectionShuffleButton = nullptr;
    bool m_collectionShuffled = false;
    AssetSort m_collectionSort;
    ImmichAlbum m_collectionAlbum; // The open album, for its order and activity.
    QPushButton *m_restoreAllButton = nullptr;
    QPushButton *m_emptyTrashButton = nullptr;
    int m_trashDays = 0;

    bool m_loadedOnce = false;
    bool m_loading = false;
};

} // namespace Aurora
