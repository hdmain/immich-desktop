#include "ui/pages/ExplorePage.h"

#include "ui/widgets/VideoPlayerDialog.h"
#include "ui/widgets/ZoomPanWidget.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QVBoxLayout>

namespace Aurora {

ExploreCard::ExploreCard(Style style, QWidget *parent)
    : QFrame(parent)
    , m_style(style)
    , m_image(new QLabel(this))
    , m_caption(new QLabel(this))
{
    setObjectName(QStringLiteral("exploreCard"));
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    m_image->setAlignment(Qt::AlignCenter);
    m_caption->setAlignment(Qt::AlignCenter);
    m_caption->setProperty("subheading", true);
    m_caption->setWordWrap(true);

    if (style == Style::Person) {
        setFixedSize(108, 136);
        m_image->setFixedSize(96, 96);
    } else if (style == Style::Place) {
        setFixedSize(168, 140);
        m_image->setFixedSize(168, 100);
    } else {
        setFixedSize(140, 140);
        m_image->setFixedSize(140, 112);
        m_caption->hide();
    }

    m_image->setStyleSheet(QStringLiteral(
        "QLabel { background: rgba(127,127,127,40); border-radius: 8px; }"));
    layout->addWidget(m_image, 0, Qt::AlignHCenter);
    layout->addWidget(m_caption);
}

void ExploreCard::setCaption(const QString &text)
{
    m_caption->setText(text);
}

void ExploreCard::setPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        m_image->clear();
        m_image->setText(QStringLiteral("…"));
        return;
    }

    if (m_style == Style::Person) {
        const QPixmap scaled = pixmap.scaled(
            m_image->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QPixmap circular(m_image->size());
        circular.fill(Qt::transparent);
        QPainter painter(&circular);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        QPainterPath path;
        path.addEllipse(circular.rect());
        painter.setClipPath(path);
        const int x = (scaled.width() - circular.width()) / 2;
        const int y = (scaled.height() - circular.height()) / 2;
        painter.drawPixmap(0, 0, scaled, x, y, circular.width(), circular.height());
        m_image->setPixmap(circular);
        m_image->setStyleSheet(QStringLiteral(
            "QLabel { background: transparent; border-radius: 48px; }"));
    } else {
        m_image->setPixmap(pixmap.scaled(m_image->size(), Qt::KeepAspectRatioByExpanding,
                                         Qt::SmoothTransformation));
    }
}

void ExploreCard::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit activated();
    QFrame::mouseReleaseEvent(event);
}

void ExploreCard::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
        event->key() == Qt::Key_Space) {
        emit activated();
        return;
    }
    QFrame::keyPressEvent(event);
}

ExplorePage::ExplorePage(ImmichClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_scrollArea(new QScrollArea(this))
    , m_content(new QWidget)
    , m_contentLayout(new QVBoxLayout(m_content))
    , m_status(new QLabel(this))
    , m_emptyState(new QLabel(this))
    , m_refreshButton(new QPushButton(tr("Refresh"), this))
{
    setObjectName(QStringLiteral("explorePage"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(16, 12, 16, 10);
    toolbarLayout->setSpacing(12);

    auto *headingColumn = new QVBoxLayout;
    headingColumn->setSpacing(2);
    auto *heading = new QLabel(tr("Explore"), toolbar);
    heading->setProperty("heading", true);
    m_status->setProperty("subheading", true);
    m_status->setText(tr("People, places, and recent media"));
    headingColumn->addWidget(heading);
    headingColumn->addWidget(m_status);
    toolbarLayout->addLayout(headingColumn, 1);
    toolbarLayout->addWidget(m_refreshButton);

    m_contentLayout->setContentsMargins(16, 8, 16, 24);
    m_contentLayout->setSpacing(18);

    auto createSection = [this](SectionRow *section, const QString &title, int rowHeight) {
        section->header = new QLabel(title, m_content);
        section->header->setProperty("section", true);
        section->header->hide();

        section->scroll = new QScrollArea(m_content);
        section->scroll->setWidgetResizable(false);
        section->scroll->setFrameShape(QFrame::NoFrame);
        section->scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        section->scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        section->scroll->setFixedHeight(rowHeight);
        section->scroll->hide();

        section->host = new QWidget;
        auto *layout = new QHBoxLayout(section->host);
        layout->setContentsMargins(0, 0, 0, 8);
        layout->setSpacing(12);
        layout->addStretch();
        section->scroll->setWidget(section->host);

        m_contentLayout->addWidget(section->header);
        m_contentLayout->addWidget(section->scroll);
    };

    createSection(&m_peopleSection, tr("People"), 168);
    createSection(&m_placesSection, tr("Places"), 168);
    createSection(&m_recentSection, tr("Recently added"), 168);
    m_contentLayout->addStretch();

    m_scrollArea->setObjectName(QStringLiteral("exploreScroll"));
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidget(m_content);

    m_emptyState->setAlignment(Qt::AlignCenter);
    m_emptyState->setWordWrap(true);
    m_emptyState->setProperty("subheading", true);
    m_emptyState->setMinimumHeight(180);

    root->addWidget(toolbar);
    root->addWidget(m_emptyState);
    root->addWidget(m_scrollArea, 1);

    connect(m_refreshButton, &QPushButton::clicked, this, &ExplorePage::refresh);
    connect(m_client, &ImmichClient::exploreLoaded, this, &ExplorePage::showExplore);
    connect(m_client, &ImmichClient::filteredAssetsLoaded, this, &ExplorePage::showFilteredAssets);
    connect(m_client, &ImmichClient::personThumbnailLoaded, this,
            &ExplorePage::showPersonThumbnail);
    connect(m_client, &ImmichClient::thumbnailLoaded, this, &ExplorePage::showAssetThumbnail);
    connect(m_client, &ImmichClient::requestFailed, this, &ExplorePage::showRequestError);
    connect(m_client, &ImmichClient::configurationChanged, this, [this](bool configured) {
        m_refreshButton->setEnabled(configured);
        if (configured) {
            m_loadedOnce = false;
            if (isVisible())
                refresh();
        } else {
            clearSections();
            updateEmptyState();
        }
    });

    m_refreshButton->setEnabled(m_client->isConfigured());
    updateEmptyState();
}

void ExplorePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (m_client->isConfigured() && !m_loadedOnce)
        refresh();
}

void ExplorePage::refresh()
{
    if (!m_client->isConfigured() || m_loading)
        return;
    m_loading = true;
    m_refreshButton->setEnabled(false);
    m_status->setText(tr("Loading explore…"));
    if (!m_client->loadExplore()) {
        m_loading = false;
        m_refreshButton->setEnabled(true);
        m_status->setText(tr("Explore could not be refreshed right now."));
    }
}

void ExplorePage::clearSections()
{
    auto clearRow = [](SectionRow *section) {
        if (!section->host)
            return;
        auto *layout = qobject_cast<QHBoxLayout *>(section->host->layout());
        if (!layout)
            return;
        while (layout->count() > 1) {
            QLayoutItem *item = layout->takeAt(0);
            if (item->widget())
                item->widget()->deleteLater();
            delete item;
        }
        section->cards.clear();
        section->header->hide();
        section->scroll->hide();
    };
    clearRow(&m_peopleSection);
    clearRow(&m_placesSection);
    clearRow(&m_recentSection);
    m_personCards.clear();
    m_assetCards.clear();
}

void ExplorePage::populateSection(SectionRow *section, bool visible)
{
    section->header->setVisible(visible);
    section->scroll->setVisible(visible);
    if (visible)
        section->host->adjustSize();
}

void ExplorePage::showExplore(const ImmichExploreData &data, bool fromCache)
{
    m_loading = false;
    m_loadedOnce = true;
    m_refreshButton->setEnabled(true);
    clearSections();

    auto *peopleLayout = qobject_cast<QHBoxLayout *>(m_peopleSection.host->layout());
    for (const ImmichPerson &person : data.people) {
        auto *card = new ExploreCard(ExploreCard::Style::Person, m_peopleSection.host);
        card->setCaption(person.name.isEmpty() ? tr("Unknown") : person.name);
        card->setPixmap({});
        peopleLayout->insertWidget(peopleLayout->count() - 1, card);
        m_peopleSection.cards.append(card);
        m_personCards.insert(person.id, card);
        connect(card, &ExploreCard::activated, this, [this, person] { openPerson(person); });
        m_client->loadPersonThumbnail(person.id);
    }
    populateSection(&m_peopleSection, !data.people.isEmpty());

    auto *placesLayout = qobject_cast<QHBoxLayout *>(m_placesSection.host->layout());
    for (const ImmichPlace &place : data.places) {
        auto *card = new ExploreCard(ExploreCard::Style::Place, m_placesSection.host);
        card->setCaption(place.city);
        card->setPixmap({});
        placesLayout->insertWidget(placesLayout->count() - 1, card);
        m_placesSection.cards.append(card);
        m_assetCards.insert(place.sampleAsset.id, card);
        connect(card, &ExploreCard::activated, this, [this, place] { openPlace(place); });
        m_client->loadThumbnail(place.sampleAsset.id);
    }
    populateSection(&m_placesSection, !data.places.isEmpty());

    auto *recentLayout = qobject_cast<QHBoxLayout *>(m_recentSection.host->layout());
    for (const ImmichAsset &asset : data.recentAssets) {
        auto *card = new ExploreCard(ExploreCard::Style::Media, m_recentSection.host);
        card->setPixmap({});
        recentLayout->insertWidget(recentLayout->count() - 1, card);
        m_recentSection.cards.append(card);
        m_assetCards.insert(asset.id, card);
        connect(card, &ExploreCard::activated, this, [this, asset] { openAsset(asset); });
        m_client->loadThumbnail(asset.id);
    }
    populateSection(&m_recentSection, !data.recentAssets.isEmpty());

    m_status->setText(
        fromCache
            ? tr("%1 people · %2 places · %3 recent · offline")
                  .arg(data.people.size())
                  .arg(data.places.size())
                  .arg(data.recentAssets.size())
            : tr("%1 people · %2 places · %3 recent")
                  .arg(data.people.size())
                  .arg(data.places.size())
                  .arg(data.recentAssets.size()));
    updateEmptyState();
}

void ExplorePage::showPersonThumbnail(const QString &personId, const QPixmap &thumbnail)
{
    const QPointer<ExploreCard> card = m_personCards.value(personId);
    if (card)
        card->setPixmap(thumbnail);
}

void ExplorePage::showAssetThumbnail(const QString &assetId, const QPixmap &thumbnail)
{
    const auto cards = m_assetCards.values(assetId);
    for (ExploreCard *raw : cards) {
        const QPointer<ExploreCard> card = raw;
        if (card)
            card->setPixmap(thumbnail);
    }
}

void ExplorePage::showRequestError(const QString &operation, const QString &message)
{
    if (operation != tr("Load explore"))
        return;
    if (m_collectionLoadingMore) {
        m_collectionLoadingMore = false;
        updateCollectionLoadMore(m_collectionNextPage);
    }
    m_loading = false;
    m_refreshButton->setEnabled(true);
    m_status->setText(tr("%1 failed: %2").arg(operation, message));
    updateEmptyState();
}

void ExplorePage::updateEmptyState()
{
    const bool empty = m_peopleSection.cards.isEmpty() && m_placesSection.cards.isEmpty() &&
                       m_recentSection.cards.isEmpty();
    m_emptyState->setVisible(empty);
    m_scrollArea->setVisible(!empty);
    if (!empty)
        return;
    m_emptyState->setText(
        m_client->isConfigured()
            ? tr("No people, places, or recent media yet.\n"
                 "Run face and location recognition on your Immich server, or add photos.")
            : tr("Connect to your Immich server in Settings → Immich Server\n"
                 "to explore people, places, and recent media."));
}

void ExplorePage::openPerson(const ImmichPerson &person)
{
    m_pendingCollectionTitle =
        person.name.isEmpty() ? tr("Person") : person.name;
    m_status->setText(tr("Loading photos of %1…").arg(m_pendingCollectionTitle));
    m_client->loadAssetsForPerson(person.id);
}

void ExplorePage::openPlace(const ImmichPlace &place)
{
    m_pendingCollectionTitle = place.city;
    m_status->setText(tr("Loading photos from %1…").arg(place.city));
    m_client->loadAssetsForCity(place.city);
}

void ExplorePage::showFilteredAssets(const QString &filterKind, const QString &filterValue,
                                     const QList<ImmichAsset> &assets, const QString &nextPage)
{
    const bool appendToOpen =
        m_collectionLoadingMore && m_collectionDialog &&
        m_collectionFilterKind == filterKind && m_collectionFilterValue == filterValue;
    m_collectionLoadingMore = false;

    if (appendToOpen) {
        appendCollectionAssets(assets);
        updateCollectionLoadMore(nextPage);
        return;
    }

    QString title = m_pendingCollectionTitle;
    if (title.isEmpty()) {
        title = filterKind == QStringLiteral("city") ? filterValue : tr("Photos");
    }
    m_pendingCollectionTitle.clear();
    openAssetCollection(title, filterKind, filterValue, assets, nextPage);
}

void ExplorePage::appendCollectionAssets(const QList<ImmichAsset> &assets)
{
    if (!m_collectionDialog || !m_collectionGrid)
        return;

    auto *host = m_collectionGrid->parentWidget();
    if (!host)
        return;

    constexpr int columns = 4;
    for (const ImmichAsset &asset : assets) {
        auto *card = new ExploreCard(ExploreCard::Style::Media, host);
        m_collectionGrid->addWidget(card, m_collectionAssetCount / columns,
                                    m_collectionAssetCount % columns);
        ++m_collectionAssetCount;
        connect(m_client, &ImmichClient::thumbnailLoaded, card,
                [card, assetId = asset.id](const QString &id, const QPixmap &thumbnail) {
                    if (id == assetId)
                        card->setPixmap(thumbnail);
                });
        connect(card, &ExploreCard::activated, this, [this, asset] { openAsset(asset); });
        m_client->loadThumbnail(asset.id);
    }
}

void ExplorePage::updateCollectionLoadMore(const QString &nextPage)
{
    m_collectionNextPage = nextPage;
    if (!m_collectionLoadMore)
        return;
    bool ok = false;
    const int page = nextPage.toInt(&ok);
    m_collectionLoadMore->setEnabled(ok && page > 0);
    m_collectionLoadMore->setVisible(ok && page > 0);
    m_collectionLoadMore->setText(tr("Load more"));
}

void ExplorePage::requestCollectionNextPage()
{
    bool ok = false;
    const int page = m_collectionNextPage.toInt(&ok);
    if (!ok || page <= 0 || m_collectionFilterKind.isEmpty() ||
        m_collectionFilterValue.isEmpty())
        return;

    m_collectionLoadingMore = true;
    if (m_collectionLoadMore) {
        m_collectionLoadMore->setEnabled(false);
        m_collectionLoadMore->setText(tr("Loading…"));
    }

    if (m_collectionFilterKind == QStringLiteral("person"))
        m_client->loadAssetsForPerson(m_collectionFilterValue, page);
    else if (m_collectionFilterKind == QStringLiteral("city"))
        m_client->loadAssetsForCity(m_collectionFilterValue, page);
    else
        m_collectionLoadingMore = false;
}

void ExplorePage::openAssetCollection(const QString &title, const QString &filterKind,
                                      const QString &filterValue,
                                      const QList<ImmichAsset> &assets,
                                      const QString &nextPage)
{
    if (m_collectionDialog)
        m_collectionDialog->close();

    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setAttribute(Qt::WA_StyledBackground, true);
    dialog->setWindowTitle(title);
    dialog->resize(900, 640);

    m_collectionDialog = dialog;
    m_collectionFilterKind = filterKind;
    m_collectionFilterValue = filterValue;
    m_collectionAssetCount = 0;
    m_collectionNextPage.clear();

    auto *layout = new QVBoxLayout(dialog);
    auto *scroll = new QScrollArea(dialog);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *host = new QWidget;
    auto *grid = new QGridLayout(host);
    grid->setContentsMargins(8, 8, 8, 8);
    grid->setSpacing(8);
    m_collectionGrid = grid;

    if (assets.isEmpty()) {
        auto *empty = new QLabel(tr("No photos found."), host);
        empty->setAlignment(Qt::AlignCenter);
        empty->setProperty("subheading", true);
        grid->addWidget(empty, 0, 0);
    } else {
        appendCollectionAssets(assets);
    }

    scroll->setWidget(host);
    layout->addWidget(scroll, 1);

    auto *loadMore = new QPushButton(tr("Load more"), dialog);
    m_collectionLoadMore = loadMore;
    connect(loadMore, &QPushButton::clicked, this, &ExplorePage::requestCollectionNextPage);
    layout->addWidget(loadMore);
    updateCollectionLoadMore(nextPage);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    layout->addWidget(buttons);

    connect(dialog, &QObject::destroyed, this, [this] {
        m_collectionGrid.clear();
        m_collectionLoadMore.clear();
        m_collectionFilterKind.clear();
        m_collectionFilterValue.clear();
        m_collectionNextPage.clear();
        m_collectionAssetCount = 0;
        m_collectionLoadingMore = false;
    });

    dialog->show();
}

void ExplorePage::openAsset(const ImmichAsset &asset)
{
    if (asset.isVideo()) {
        auto *player = new VideoPlayerDialog(m_client, asset, this);
        player->show();
        return;
    }

    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setAttribute(Qt::WA_StyledBackground, true);
    dialog->setWindowTitle(asset.fileName.isEmpty() ? tr("Media preview") : asset.fileName);
    dialog->resize(960, 700);

    auto *layout = new QVBoxLayout(dialog);
    auto *preview = new ZoomPanWidget(dialog);
    preview->setPlaceholderText(tr("Loading preview…"));
    preview->setMinimumSize(480, 320);
    layout->addWidget(preview, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    layout->addWidget(buttons);

    connect(m_client, &ImmichClient::previewLoaded, dialog,
            [preview, id = asset.id](const QString &assetId, const QPixmap &pixmap) {
                if (assetId != id)
                    return;
                preview->setPixmap(pixmap);
            });
    m_client->loadPreview(asset.id);
    dialog->show();
}

} // namespace Aurora
