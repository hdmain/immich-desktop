#include "ui/widgets/VideoHoverPreview.h"

#include "core/AppSettings.h"
#include "ui/widgets/MediaTile.h"
#include "ui/widgets/SoftwareVideoWidget.h"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>

namespace Aurora {

namespace {
constexpr int kHoverPreviewMs = 2500;
constexpr int kHoverStartDelayMs = 350;
} // namespace

VideoHoverPreview::VideoHoverPreview(ImmichClient *client, QWidget *hostWidget,
                                     QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_hostWidget(hostWidget)
    , m_overlay(new SoftwareVideoWidget(m_hostWidget))
    , m_player(new QMediaPlayer(this))
    , m_audio(new QAudioOutput(this))
    , m_sink(new QVideoSink(this))
    , m_startTimer(new QTimer(this))
    , m_stopTimer(new QTimer(this))
{
    m_overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_overlay->setScaleMode(SoftwareVideoWidget::ScaleMode::Cover);
    m_overlay->hide();

    m_audio->setVolume(0.0f);
    m_player->setAudioOutput(m_audio);
    m_player->setVideoOutput(m_sink);

    m_startTimer->setSingleShot(true);
    m_stopTimer->setSingleShot(true);

    connect(m_startTimer, &QTimer::timeout, this, [this] {
        MediaTile *tile = m_pendingTile.data();
        const QUrl url = m_pendingUrl;
        m_pendingTile = nullptr;
        m_pendingUrl = QUrl();
        if (!tile || !url.isValid())
            return;

        m_activeTile = tile;
        m_overlay->setParent(tile);
        m_overlay->clearFrame();
        updateTileGeometry(tile);
        m_overlay->raise();
        m_overlay->show();
        tile->update();

        beginPlayback(url);
        m_stopTimer->start(kHoverPreviewMs);
    });

    connect(m_stopTimer, &QTimer::timeout, this, &VideoHoverPreview::stop);

    connect(m_sink, &QVideoSink::videoFrameChanged, this,
            [this](const QVideoFrame &frame) {
                if (!m_activeTile || !m_overlay->isVisible())
                    return;
                QVideoFrame copy(frame);
                const QImage image = copy.toImage();
                if (!image.isNull())
                    m_overlay->setFrame(image);
            });

    connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
                if (!m_activeTile || m_handlingPlayer)
                    return;
                if (status != QMediaPlayer::LoadedMedia &&
                    status != QMediaPlayer::BufferedMedia)
                    return;

                m_handlingPlayer = true;
                m_player->setPosition(0);
                m_player->play();
                m_handlingPlayer = false;
            });

    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (!m_activeTile || m_handlingPlayer || position < kHoverPreviewMs)
            return;
        scheduleStop();
    });

    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &) { scheduleStop(); });
}

void VideoHoverPreview::showForTile(MediaTile *tile)
{
    if (!m_enabled)
        return;
    if (!AppSettings().loadPlayback().hoverPreviewEnabled)
        return;
    if (!tile || !m_client || !tile->asset().isVideo())
        return;

    const QUrl streamUrl = m_client->videoStreamUrl(tile->asset().id);
    if (!streamUrl.isValid())
        return;

    if (m_activeTile.data() == tile && m_loadedUrl == streamUrl &&
        m_player->playbackState() == QMediaPlayer::PlayingState)
        return;

    armStart(tile, streamUrl);
}

void VideoHoverPreview::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled)
        stop();
}

void VideoHoverPreview::hideForTile(MediaTile *tile)
{
    if (m_pendingTile.data() == tile) {
        m_startTimer->stop();
        m_pendingTile = nullptr;
        m_pendingUrl = QUrl();
    }
    if (m_activeTile.data() != tile)
        return;
    scheduleStop();
}

void VideoHoverPreview::updateTileGeometry(MediaTile *tile)
{
    if (!tile || m_activeTile.data() != tile || !m_overlay)
        return;
    m_overlay->setGeometry(tile->rect());
}

void VideoHoverPreview::armStart(MediaTile *tile, const QUrl &streamUrl)
{
    m_stopTimer->stop();
    m_startTimer->stop();

    if (m_activeTile && m_activeTile.data() != tile)
        stop();

    m_pendingTile = tile;
    m_pendingUrl = streamUrl;
    m_startTimer->start(kHoverStartDelayMs);
}

void VideoHoverPreview::beginPlayback(const QUrl &streamUrl)
{
    m_handlingPlayer = true;

    const bool sameSource = m_loadedUrl == streamUrl;
    const QMediaPlayer::MediaStatus status = m_player->mediaStatus();
    const bool mediaReady = status == QMediaPlayer::LoadedMedia ||
                            status == QMediaPlayer::BufferedMedia;

    if (sameSource && mediaReady) {
        m_player->setPosition(0);
        m_player->play();
    } else {
        m_player->stop();
        m_loadedUrl = streamUrl;
        QTimer::singleShot(0, this, [this, streamUrl] {
            if (!m_activeTile || m_loadedUrl != streamUrl)
                return;
            m_player->setSource(streamUrl);
            m_player->play();
        });
    }

    m_handlingPlayer = false;
}

void VideoHoverPreview::scheduleStop()
{
    if (m_handlingPlayer)
        return;
    QTimer::singleShot(0, this, &VideoHoverPreview::stop);
}

void VideoHoverPreview::detachOverlay()
{
    if (!m_overlay)
        return;
    m_overlay->hide();
    m_overlay->clearFrame();
    if (m_overlay->parentWidget() != m_hostWidget)
        m_overlay->setParent(m_hostWidget);
}

void VideoHoverPreview::stop()
{
    m_startTimer->stop();
    m_stopTimer->stop();
    m_pendingTile = nullptr;
    m_pendingUrl = QUrl();

    MediaTile *tile = m_activeTile.data();
    m_activeTile = nullptr;

    if (tile)
        tile->endHoverPreview();

    m_handlingPlayer = true;
    if (m_player)
        m_player->stop();
    m_handlingPlayer = false;

    detachOverlay();
}

} // namespace Aurora
