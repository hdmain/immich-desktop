#include "ui/widgets/MediaViewerDialog.h"

#include "core/AppSettings.h"
#include "ui/widgets/AnimatedButton.h"
#include "ui/widgets/AssetInfoPanel.h"
#include "ui/widgets/ShareLinkDialog.h"
#include "ui/widgets/Toast.h"
#include "ui/widgets/VideoPlayerWidget.h"
#include "ui/widgets/ZoomPanWidget.h"

#include <QHBoxLayout>
#include <QKeySequence>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace Aurora {

MediaViewerDialog::MediaViewerDialog(ThemeManager *themeManager, ImmichClient *client,
                                     const ImmichAsset &asset, NeighborProvider neighbors,
                                     QWidget *parent)
    : QDialog(parent)
    , m_client(client)
    , m_neighbors(std::move(neighbors))
    , m_stack(new QStackedWidget(this))
    , m_preview(new ZoomPanWidget(this))
    , m_videoPlayer(new VideoPlayerWidget(client, this))
    , m_infoPanel(new AssetInfoPanel(client, this))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_StyledBackground, true);
    resize(960, 700);

    m_viewTimer = new QTimer(this);
    m_viewTimer->setSingleShot(true);
    m_viewTimer->setInterval(2000);
    connect(m_viewTimer, &QTimer::timeout, this, [this] {
        if (!m_trashMode)
            m_client->recordViewed(m_asset);
    });

    m_preview->setMinimumSize(480, 320);
    m_stack->addWidget(m_preview);
    m_stack->addWidget(m_videoPlayer);

    auto makeButton = [this, themeManager](const QString &label, const QString &icon) {
        auto *button = new AnimatedButton(label, icon, themeManager, this);
        button->setCheckable(false);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        return button;
    };
    m_previousButton = makeButton({}, QStringLiteral(":/icons/chevron-left.svg"));
    m_nextButton = makeButton({}, QStringLiteral(":/icons/chevron-right.svg"));
    m_favoriteButton = makeButton(tr("Favorite"), QStringLiteral(":/icons/heart.svg"));
    m_copyButton = makeButton(tr("Copy"), QStringLiteral(":/icons/copy.svg"));
    auto *downloadButton = makeButton(tr("Download"), QStringLiteral(":/icons/download.svg"));
    m_shareButton = makeButton(tr("Share link"), QStringLiteral(":/icons/external-link.svg"));
    m_trashButton = makeButton(tr("Move to trash"), QStringLiteral(":/icons/trash.svg"));
    m_restoreButton = makeButton(tr("Restore"), QStringLiteral(":/icons/rotate-ccw.svg"));
    m_restoreButton->hide();
    auto *deleteButton =
        makeButton(tr("Delete permanently"), QStringLiteral(":/icons/circle-x.svg"));
    m_infoButton = makeButton(tr("Info"), QStringLiteral(":/icons/info.svg"));
    m_infoButton->setCheckable(true);
    m_infoButton->setToolTip(tr("Show info (I)"));
    auto *closeButton = makeButton(tr("Close"), QStringLiteral(":/icons/x.svg"));
    m_previousButton->setToolTip(tr("Previous (Left arrow)"));
    m_nextButton->setToolTip(tr("Next (Right arrow)"));

    auto *controls = new QHBoxLayout;
    controls->addWidget(m_previousButton);
    controls->addWidget(m_nextButton);
    controls->addStretch(1);
    controls->addWidget(m_favoriteButton);
    controls->addWidget(m_copyButton);
    controls->addWidget(downloadButton);
    controls->addWidget(m_shareButton);
    controls->addWidget(m_trashButton);
    controls->addWidget(m_restoreButton);
    controls->addWidget(deleteButton);
    controls->addWidget(m_infoButton);
    controls->addWidget(closeButton);

    auto *content = new QHBoxLayout;
    content->setSpacing(0);
    content->addWidget(m_stack, 1);
    content->addWidget(m_infoPanel);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(content, 1);
    layout->addLayout(controls);

    connect(m_previousButton, &QPushButton::clicked, this, [this] { step(-1); });
    connect(m_nextButton, &QPushButton::clicked, this, [this] { step(1); });
    connect(m_favoriteButton, &QPushButton::clicked, this,
            [this] { m_client->setFavorite({m_asset.id}, !m_asset.favorite); });
    connect(m_client, &ImmichClient::favoritesChanged, this,
            [this](const QStringList &assetIds, bool favorite) {
                for (const QString &id : assetIds)
                    m_favoriteChanges.insert(id, favorite);
                if (assetIds.contains(m_asset.id)) {
                    m_asset.favorite = favorite;
                    updateFavoriteButton();
                }
            });
    connect(m_client, &ImmichClient::favoriteFailed, this,
            [this](const QStringList &assetIds, const QString &message) {
                if (assetIds.contains(m_asset.id))
                    Toast::showMessage(this, message);
            });
    connect(m_copyButton, &QPushButton::clicked, this, [this] { emit copyRequested(m_asset); });
    connect(downloadButton, &QPushButton::clicked, this,
            [this] { emit downloadRequested(m_asset); });
    connect(m_shareButton, &QPushButton::clicked, this,
            [this] { (new ShareLinkDialog(m_client, m_asset, this))->show(); });
    connect(m_trashButton, &QPushButton::clicked, this, [this] { emit trashRequested(m_asset); });
    connect(m_restoreButton, &QPushButton::clicked, this,
            [this] { emit restoreRequested(m_asset); });
    connect(deleteButton, &QPushButton::clicked, this,
            [this] { emit deleteRequested(m_asset); });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_infoButton, &QPushButton::toggled, this, [this](bool open) {
        setInfoPanelVisible(open);
        WindowSettings window = AppSettings().loadWindow();
        window.viewerInfoOpen = open;
        AppSettings().saveWindow(window);
    });
    connect(m_infoPanel, &AssetInfoPanel::closeRequested, m_infoButton,
            [this] { m_infoButton->setChecked(false); });
    connect(m_infoPanel, &AssetInfoPanel::personRequested, this,
            &MediaViewerDialog::personRequested);
    connect(m_infoPanel, &AssetInfoPanel::placeRequested, this,
            &MediaViewerDialog::placeRequested);
    connect(m_infoPanel, &AssetInfoPanel::dateRequested, this,
            &MediaViewerDialog::dateRequested);
    connect(m_infoPanel, &AssetInfoPanel::albumRequested, this,
            &MediaViewerDialog::albumRequested);
    connect(m_infoPanel, &AssetInfoPanel::tagRequested, this,
            &MediaViewerDialog::tagRequested);
    connect(m_infoPanel, &AssetInfoPanel::folderRequested, this,
            &MediaViewerDialog::folderRequested);
    connect(m_client, &ImmichClient::assetDetailsLoaded, this,
            [this](const ImmichAssetDetails &details) {
                if (details.asset.id == m_asset.id && !details.asset.fileName.isEmpty())
                    setWindowTitle(details.asset.fileName);
            });

    auto *previousShortcut = new QShortcut(QKeySequence(Qt::Key_Left), this);
    connect(previousShortcut, &QShortcut::activated, this, [this] { step(-1); });
    auto *nextShortcut = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(nextShortcut, &QShortcut::activated, this, [this] { step(1); });
    auto *favoriteShortcut = new QShortcut(QKeySequence(Qt::Key_F), this);
    connect(favoriteShortcut, &QShortcut::activated, m_favoriteButton, [this] {
        if (m_favoriteButton->isVisible())
            m_favoriteButton->click();
    });
    auto *infoShortcut = new QShortcut(QKeySequence(Qt::Key_I), this);
    connect(infoShortcut, &QShortcut::activated, m_infoButton, &QPushButton::toggle);

    connect(m_client, &ImmichClient::previewLoaded, this,
            [this](const QString &assetId, const QPixmap &pixmap) {
                if (assetId != m_asset.id)
                    return;
                if (m_asset.isVideo())
                    m_videoPlayer->setPoster(pixmap);
                else
                    m_preview->setPixmap(pixmap);
            });
    connect(m_client, &ImmichClient::imageLoadFailed, this,
            [this](const QString &assetId, const QString &resultSize, const QString &message) {
                if (assetId == m_asset.id && resultSize == QStringLiteral("preview"))
                    m_preview->setPlaceholderText(tr("Preview unavailable\n%1").arg(message));
            });
    connect(m_client, &ImmichClient::assetsDeleted, this,
            [this](const QStringList &assetIds, bool) { handleAssetsDeleted(assetIds); });
    connect(m_client, &ImmichClient::assetsRestored, this, [this](const QStringList &assetIds) {
        // A restored photo leaves the trash, so step on like after a delete.
        if (m_trashMode && !assetIds.isEmpty())
            handleAssetsDeleted(assetIds);
    });
    connect(m_client, &ImmichClient::timelineBucketLoaded, this,
            &MediaViewerDialog::updateNavigation);
    connect(m_client, &ImmichClient::assetsLoaded, this, &MediaViewerDialog::updateNavigation);
    connect(m_client, &ImmichClient::filteredAssetsLoaded, this,
            &MediaViewerDialog::updateNavigation);

    // Only the item opened from the grid autoplays; videos reached by browsing wait.
    setAsset(asset, asset.isVideo());
    setInfoPanelVisible(AppSettings().loadWindow().viewerInfoOpen);
}

void MediaViewerDialog::updateNavigation()
{
    m_previousAsset = m_neighbors(m_asset.id, -1);
    m_nextAsset = m_neighbors(m_asset.id, 1);
    m_previousButton->setEnabled(!m_previousAsset.id.isEmpty());
    m_nextButton->setEnabled(!m_nextAsset.id.isEmpty());

    const ImmichAsset &ahead = m_lastDirection < 0 ? m_previousAsset : m_nextAsset;
    if (!ahead.id.isEmpty())
        m_client->loadPreview(ahead.id);
}

void MediaViewerDialog::setAsset(const ImmichAsset &asset, bool autoplay)
{
    m_asset = asset;
    m_asset.favorite = m_favoriteChanges.value(asset.id, asset.favorite);
    setWindowTitle(asset.fileName.isEmpty() ? tr("Media preview") : asset.fileName);
    m_copyButton->setVisible(!asset.isVideo() && !m_trashMode);
    updateFavoriteButton();
    if (asset.isVideo()) {
        m_videoPlayer->setAsset(asset, autoplay);
        m_stack->setCurrentWidget(m_videoPlayer);
    } else {
        m_videoPlayer->stop();
        m_preview->setPixmap({});
        m_preview->setPlaceholderText(tr("Loading preview…"));
        m_stack->setCurrentWidget(m_preview);
    }
    m_client->loadPreview(asset.id);
    m_infoPanel->setAsset(asset);
    updateNavigation();
    // A photo counts as viewed once it has been open a moment, so skimming
    // through doesn't count. The trash isn't history.
    m_viewTimer->start();
}

void MediaViewerDialog::setTrashMode(bool trashMode)
{
    m_trashMode = trashMode;
    m_copyButton->setVisible(!m_asset.isVideo() && !trashMode);
    m_shareButton->setVisible(!trashMode);
    m_trashButton->setVisible(!trashMode);
    m_restoreButton->setVisible(trashMode);
    updateFavoriteButton();
}

void MediaViewerDialog::updateFavoriteButton()
{
    m_favoriteButton->setVisible(!m_trashMode && !m_asset.isLocalPending());
    m_favoriteButton->setIconResource(m_asset.favorite ? QStringLiteral(":/icons/heart-filled.svg")
                                                       : QStringLiteral(":/icons/heart.svg"));
    m_favoriteButton->setToolTip(m_asset.favorite ? tr("Remove from favorites (F)")
                                                  : tr("Add to favorites (F)"));
}

void MediaViewerDialog::setInfoPanelVisible(bool visible)
{
    // Only the Info button and I key change the remembered preference.
    const QSignalBlocker blocker(m_infoButton);
    m_infoButton->setChecked(visible);
    m_infoPanel->setVisible(visible);
}

void MediaViewerDialog::step(int direction)
{
    const ImmichAsset target = m_neighbors(m_asset.id, direction);
    if (target.id.isEmpty())
        return;
    m_lastDirection = direction;
    setAsset(target);
}

void MediaViewerDialog::handleAssetsDeleted(const QStringList &assetIds)
{
    if (!assetIds.contains(m_asset.id)) {
        updateNavigation();
        return;
    }
    const ImmichAsset &forward = m_lastDirection < 0 ? m_previousAsset : m_nextAsset;
    const ImmichAsset &backward = m_lastDirection < 0 ? m_nextAsset : m_previousAsset;
    for (const ImmichAsset &candidate : {forward, backward}) {
        if (!candidate.id.isEmpty() && !assetIds.contains(candidate.id)) {
            setAsset(candidate);
            return;
        }
    }
    close();
}

} // namespace Aurora
