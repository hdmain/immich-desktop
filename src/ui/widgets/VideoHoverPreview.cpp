#include "ui/widgets/VideoHoverPreview.h"

#include "ui/widgets/MediaTile.h"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QTimer>
#include <QVideoWidget>

namespace Aurora {

namespace {
constexpr int kHoverPreviewMs = 2500;
} // namespace

VideoHoverPreview::VideoHoverPreview(ImmichClient *client, QWidget *hostWidget,
                                     QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_hostWidget(hostWidget)
    , m_video(new QVideoWidget(m_hostWidget))
    , m_player(new QMediaPlayer(this))
    , m_audio(new QAudioOutput(this))
    , m_stopTimer(new QTimer(this))
{
    m_video->hide();
    m_video->setStyleSheet(QStringLiteral("background: black;"));

    m_audio->setVolume(0.0f);
    m_player->setAudioOutput(m_audio);
    m_player->setVideoOutput(m_video);

    m_stopTimer->setSingleShot(true);
    connect(m_stopTimer, &QTimer::timeout, this, &VideoHoverPreview::stop);

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
    if (!tile || !m_client || !tile->asset().isVideo())
        return;

    const QUrl streamUrl = m_client->videoStreamUrl(tile->asset().id);
    if (!streamUrl.isValid())
        return;

    if (m_activeTile && m_activeTile.data() != tile)
        stop();

    m_activeTile = tile;
    m_video->setParent(tile);
    updateTileGeometry(tile);
    m_video->raise();
    m_video->show();
    tile->update();

    beginPlayback(streamUrl);
    m_stopTimer->start(kHoverPreviewMs);
}

void VideoHoverPreview::hideForTile(MediaTile *tile)
{
    if (m_activeTile.data() != tile)
        return;
    scheduleStop();
}

void VideoHoverPreview::updateTileGeometry(MediaTile *tile)
{
    if (!tile || m_activeTile.data() != tile)
        return;
    m_video->setGeometry(tile->rect());
}

void VideoHoverPreview::beginPlayback(const QUrl &streamUrl)
{
    m_handlingPlayer = true;
    m_player->blockSignals(true);

    const bool sameSource = m_player->source() == streamUrl;
    const QMediaPlayer::MediaStatus status = m_player->mediaStatus();
    const bool mediaReady = status == QMediaPlayer::LoadedMedia ||
                            status == QMediaPlayer::BufferedMedia;

    if (sameSource && mediaReady) {
        m_player->setPosition(0);
        m_player->play();
    } else {
        m_player->stop();
        m_player->setSource(streamUrl);
    }

    m_player->blockSignals(false);
    m_handlingPlayer = false;
}

void VideoHoverPreview::scheduleStop()
{
    if (m_handlingPlayer)
        return;
    QTimer::singleShot(0, this, &VideoHoverPreview::stop);
}

void VideoHoverPreview::stop()
{
    m_stopTimer->stop();

    MediaTile *tile = m_activeTile.data();
    m_activeTile = nullptr;

    if (tile)
        tile->endHoverPreview();

    if (!m_player && !m_video)
        return;

    m_handlingPlayer = true;

    if (m_player) {
        m_player->blockSignals(true);
        m_player->stop();
        m_player->setSource(QUrl());
        m_player->blockSignals(false);
    }

    if (m_video) {
        m_video->hide();
        m_video->setParent(m_hostWidget);
    }

    m_handlingPlayer = false;
}

} // namespace Aurora
