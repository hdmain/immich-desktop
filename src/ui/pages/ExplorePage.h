#pragma once

#include "core/ImmichClient.h"

#include <QFrame>
#include <QHash>
#include <QList>
#include <QMultiHash>
#include <QPixmap>
#include <QPointer>
#include <QWidget>

class QGridLayout;
class QKeyEvent;
class QLabel;
class QMouseEvent;
class QPushButton;
class QScrollArea;
class QShowEvent;
class QStackedWidget;
class QVBoxLayout;

namespace Aurora {

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

private:
    enum class Page : int { Browse = 0, Collection = 1, Preview = 2 };

    struct SectionRow {
        QLabel *header = nullptr;
        QScrollArea *scroll = nullptr;
        QWidget *host = nullptr;
        QList<ExploreCard *> cards;
    };

    void clearSections();
    void updateEmptyState();
    void populateSection(SectionRow *section, bool visible);
    void showCollectionPage(const QString &title, const QString &subtitle,
                            const QString &filterKind, const QString &filterValue,
                            const QPixmap &heroThumb, bool personStyle);
    void clearCollectionGrid();
    void appendCollectionAssets(const QList<ImmichAsset> &assets);
    void updateCollectionLoadMore(const QString &nextPage);
    void showPreviewPage(const ImmichAsset &asset);
    void setPage(Page page);

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
    QMultiHash<QString, ExploreCard *> m_assetCards;

    // Collection detail
    QWidget *m_collectionPage = nullptr;
    QPushButton *m_collectionBack = nullptr;
    QLabel *m_collectionHero = nullptr;
    QLabel *m_collectionTitle = nullptr;
    QLabel *m_collectionSubtitle = nullptr;
    QScrollArea *m_collectionScroll = nullptr;
    QWidget *m_collectionHost = nullptr;
    QGridLayout *m_collectionGrid = nullptr;
    QPushButton *m_collectionLoadMore = nullptr;
    QString m_collectionFilterKind;
    QString m_collectionFilterValue;
    QString m_collectionHeroAssetId;
    QString m_collectionNextPage;
    QString m_pendingCollectionTitle;
    QPixmap m_pendingHeroThumb;
    bool m_pendingPersonStyle = false;
    int m_collectionAssetCount = 0;
    bool m_collectionLoadingMore = false;
    bool m_collectionPersonStyle = false;

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
