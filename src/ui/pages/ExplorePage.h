#pragma once

#include "core/ImmichClient.h"

#include <QFrame>
#include <QHash>
#include <QList>
#include <QMultiHash>
#include <QWidget>

class QKeyEvent;
class QLabel;
class QMouseEvent;
class QPushButton;
class QScrollArea;
class QShowEvent;
class QVBoxLayout;

namespace Aurora {

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
    void showRequestError(const QString &operation, const QString &message);
    void openPerson(const Aurora::ImmichPerson &person);
    void openPlace(const Aurora::ImmichPlace &place);
    void openAsset(const Aurora::ImmichAsset &asset);

private:
    struct SectionRow {
        QLabel *header = nullptr;
        QScrollArea *scroll = nullptr;
        QWidget *host = nullptr;
        QList<ExploreCard *> cards;
    };

    void clearSections();
    void updateEmptyState();
    void openAssetCollection(const QString &title, const QList<ImmichAsset> &assets);
    void populateSection(SectionRow *section, bool visible);

    ImmichClient *m_client;
    QScrollArea *m_scrollArea;
    QWidget *m_content;
    QVBoxLayout *m_contentLayout;
    QLabel *m_status;
    QLabel *m_emptyState;
    QPushButton *m_refreshButton;
    SectionRow m_peopleSection;
    SectionRow m_placesSection;
    SectionRow m_recentSection;
    QHash<QString, ExploreCard *> m_personCards;
    QMultiHash<QString, ExploreCard *> m_assetCards;
    QString m_pendingCollectionTitle;
    bool m_loadedOnce = false;
    bool m_loading = false;
};

} // namespace Aurora
