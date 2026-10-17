#include "ui/widgets/MediaViewerDialog.h"

#include "ui/widgets/AnimatedButton.h"
#include "ui/widgets/ShareLinkDialog.h"
#include "ui/widgets/VideoPlayerWidget.h"
#include "ui/widgets/ZoomPanWidget.h"

#include <QHBoxLayout>
#include <QKeySequence>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
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
{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_StyledBackground, true);
    resize(960, 700);

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
    m_copyButton = makeButton(tr("Copy"), QStringLiteral(":/icons/copy.svg"));
    auto *downloadButton = makeButton(tr("Download"), QStringLiteral(":/icons/download.svg"));
    auto *shareButton = makeButton(tr("Share link"), QStringLiteral(":/icons/external-link.svg"));
    auto *trashButton = makeButton(tr("Move to trash"), QStringLiteral(":/icons/trash.svg"));
    auto *deleteButton =
        makeButton(tr("Delete permanently"), QStringLiteral(":/icons/circle-x.svg"));
    auto *closeButton = makeButton(tr("Close"), QStringLiteral(":/icons/x.svg"));
    m_previousButton->setToolTip(tr("Previous (Left arrow)"));
    m_nextButton->setToolTip(tr("Next (Right arrow)"));

    auto *controls = new QHBoxLayout;
    controls->addWidget(m_previousButton);
    controls->addWidget(m_nextButton);
    controls->addStretch(1);
    controls->addWidget(m_copyButton);
    controls->addWidget(downloadButton);
    controls->addWidget(shareButton);
    controls->addWidget(trashButton);
    controls->addWidget(deleteButton);
    controls->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_stack, 1);
    layout->addLayout(controls);

    connect(m_previousButton, &QPushButton::clicked, this, [this] { step(-1); });
    connect(m_nextButton, &QPushButton::clicked, this, [this] { step(1); });
    connect(m_copyButton, &QPushButton::clicked, this, [this] { emit copyRequested(m_asset); });
    connect(downloadButton, &QPushButton::clicked, this,
            [this] { emit downloadRequested(m_asset); });
    connect(shareButton, &QPushButton::clicked, this,
            [this] { (new ShareLinkDialog(m_client, m_asset, this))->show(); });
    connect(trashButton, &QPushButton::clicked, this, [this] { emit trashRequested(m_asset); });
    connect(deleteButton, &QPushButton::clicked, this,
            [this] { emit deleteRequested(m_asset); });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

    auto *previousShortcut = new QShortcut(QKeySequence(Qt::Key_Left), this);
    connect(previousShortcut, &QShortcut::activated, this, [this] { step(-1); });
    auto *nextShortcut = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(nextShortcut, &QShortcut::activated, this, [this] { step(1); });

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
    connect(m_client, &ImmichClient::timelineBucketLoaded, this,
            &MediaViewerDialog::updateNavigation);
    connect(m_client, &ImmichClient::assetsLoaded, this, &MediaViewerDialog::updateNavigation);
    connect(m_client, &ImmichClient::filteredAssetsLoaded, this,
            &MediaViewerDialog::updateNavigation);

    // Only the item opened from the grid autoplays; videos reached by browsing wait.
    setAsset(asset, asset.isVideo());
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
    setWindowTitle(asset.fileName.isEmpty() ? tr("Media preview") : asset.fileName);
    m_copyButton->setVisible(!asset.isVideo());
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
    updateNavigation();
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
