#include "ui/widgets/VideoHoverPreview.h"

#include "core/AppSettings.h"
#include "ui/widgets/MediaTile.h"
#include "ui/widgets/SoftwareVideoWidget.h"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QMetaObject>
#include <QThread>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>

namespace Aurora {

namespace {
constexpr int kHoverPreviewMs = 2500;
constexpr int kHoverStartDelayMs = 450;
constexpr int kFrameUiIntervalMs = 100; // 10 fps max onto the UI thread
constexpr int kWorkerFrameMaxEdge = 240;
} // namespace

// Persistent worker: one QThread for all hover previews (no per-hover thread churn).
class HoverPreviewEngine final : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

public slots:
    void play(const QUrl &url)
    {
        ensurePlayer();
        m_player->stop();
        m_player->setSource(url);
        m_player->play();
    }

    void stop()
    {
        if (!m_player)
            return;
        m_player->stop();
        m_player->setSource(QUrl());
    }

signals:
    void frameReady(const QImage &image);
    void failed();
    void timedOut();

private:
    void ensurePlayer()
    {
        if (m_player)
            return;

        m_player = new QMediaPlayer(this);
        m_audio = new QAudioOutput(this);
        m_sink = new QVideoSink(this);
        m_audio->setVolume(0.0f);
        m_player->setAudioOutput(m_audio);
        m_player->setVideoOutput(m_sink);

        connect(m_sink, &QVideoSink::videoFrameChanged, this,
                [this](const QVideoFrame &frame) {
                    QVideoFrame copy(frame);
                    QImage image = copy.toImage();
                    if (image.isNull())
                        return;
                    if (image.width() > kWorkerFrameMaxEdge ||
                        image.height() > kWorkerFrameMaxEdge) {
                        image = image.scaled(kWorkerFrameMaxEdge, kWorkerFrameMaxEdge,
                                             Qt::KeepAspectRatio, Qt::FastTransformation);
                    }
                    // RGB32 copies cheaper on the UI painter path.
                    if (image.format() != QImage::Format_RGB32)
                        image = image.convertToFormat(QImage::Format_RGB32);
                    emit frameReady(image);
                });
        connect(m_player, &QMediaPlayer::errorOccurred, this,
                [this](QMediaPlayer::Error, const QString &) { emit failed(); });
        connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
            if (position >= kHoverPreviewMs)
                emit timedOut();
        });
        connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
                [this](QMediaPlayer::MediaStatus status) {
                    if (status == QMediaPlayer::LoadedMedia ||
                        status == QMediaPlayer::BufferedMedia) {
                        m_player->setPosition(0);
                        if (m_player->playbackState() != QMediaPlayer::PlayingState)
                            m_player->play();
                    }
                });
    }

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audio = nullptr;
    QVideoSink *m_sink = nullptr;
};

VideoHoverPreview::VideoHoverPreview(ImmichClient *client, QWidget *hostWidget,
                                     QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_hostWidget(hostWidget)
    , m_overlay(new SoftwareVideoWidget(m_hostWidget))
    , m_startTimer(new QTimer(this))
    , m_stopTimer(new QTimer(this))
    , m_frameTimer(new QTimer(this))
{
    m_overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_overlay->setScaleMode(SoftwareVideoWidget::ScaleMode::Cover);
    m_overlay->hide();

    m_startTimer->setSingleShot(true);
    m_stopTimer->setSingleShot(true);
    m_frameTimer->setSingleShot(true);
    m_frameTimer->setInterval(kFrameUiIntervalMs);

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
    connect(m_frameTimer, &QTimer::timeout, this, &VideoHoverPreview::flushPendingFrame);

    ensureWorker();
}

VideoHoverPreview::~VideoHoverPreview()
{
    stopWorkerPlayback();
    if (m_thread) {
        m_thread->quit();
        m_thread->wait(1500);
    }
}

void VideoHoverPreview::ensureWorker()
{
    if (m_thread)
        return;

    m_thread = new QThread(this);
    m_engine = new HoverPreviewEngine;
    m_engine->moveToThread(m_thread);

    connect(m_engine, &HoverPreviewEngine::frameReady, this,
            [this](const QImage &image) {
                m_pendingFrame = image;
                if (!m_frameTimer->isActive())
                    m_frameTimer->start();
            },
            Qt::QueuedConnection);
    connect(m_engine, &HoverPreviewEngine::failed, this, &VideoHoverPreview::scheduleStop,
            Qt::QueuedConnection);
    connect(m_engine, &HoverPreviewEngine::timedOut, this, &VideoHoverPreview::scheduleStop,
            Qt::QueuedConnection);
    connect(m_thread, &QThread::finished, m_engine, &QObject::deleteLater);

    m_thread->start();
}

void VideoHoverPreview::stopWorkerPlayback()
{
    m_frameTimer->stop();
    m_pendingFrame = QImage();
    m_loadedUrl = QUrl();
    if (m_engine)
        QMetaObject::invokeMethod(m_engine, "stop", Qt::QueuedConnection);
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

    if (m_activeTile.data() == tile && m_loadedUrl == streamUrl)
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

    if (m_activeTile && m_activeTile.data() != tile) {
        MediaTile *previous = m_activeTile.data();
        m_activeTile = nullptr;
        stopWorkerPlayback();
        detachOverlay();
        if (previous)
            previous->endHoverPreview();
    }

    m_pendingTile = tile;
    m_pendingUrl = streamUrl;
    m_startTimer->start(kHoverStartDelayMs);
}

void VideoHoverPreview::beginPlayback(const QUrl &streamUrl)
{
    ensureWorker();
    m_loadedUrl = streamUrl;
    QMetaObject::invokeMethod(m_engine, "play", Qt::QueuedConnection,
                              Q_ARG(QUrl, streamUrl));
}

void VideoHoverPreview::flushPendingFrame()
{
    if (m_pendingFrame.isNull() || !m_activeTile || !m_overlay || !m_overlay->isVisible())
        return;
    m_overlay->setFrame(std::move(m_pendingFrame));
    m_pendingFrame = QImage();
}

void VideoHoverPreview::scheduleStop()
{
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

    stopWorkerPlayback();

    if (tile)
        tile->endHoverPreview();

    detachOverlay();
}

} // namespace Aurora

#include "VideoHoverPreview.moc"
