#include "ui/widgets/VideoPlayerWidget.h"
#include "ui/IconUtils.h"

#include "ui/widgets/SoftwareVideoWidget.h"
#include "ui/widgets/ZoomPanWidget.h"

#include <QAudioOutput>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaPlayer>
#include <QPushButton>
#include <QResizeEvent>
#include <QShortcut>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <QVideoSink>

namespace Aurora {

VideoPlayerWidget::VideoPlayerWidget(ImmichClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_player(new QMediaPlayer(this))
    , m_audio(new QAudioOutput(this))
    , m_sink(new QVideoSink(this))
    , m_video(new SoftwareVideoWidget(this))
    , m_zoomView(new ZoomPanWidget(this))
    , m_overlayButton(new QPushButton(this))
    , m_playButton(new QPushButton(this))
    , m_positionSlider(new QSlider(Qt::Horizontal, this))
    , m_volumeSlider(new QSlider(Qt::Horizontal, this))
    , m_timeLabel(new QLabel(QStringLiteral("0:00 / 0:00"), this))
    , m_statusLabel(new QLabel(this))
{
    m_video->setMinimumSize(640, 360);
    m_player->setAudioOutput(m_audio);
    m_player->setVideoOutput(m_sink);
    m_audio->setVolume(0.8f);

    m_zoomView->setHostedWidget(m_video);
    m_zoomView->setMinimumSize(640, 360);

    m_overlayButton->setObjectName(QStringLiteral("videoOverlayButton"));
    m_overlayButton->setFixedSize(72, 72);
    m_overlayButton->setIconSize(QSize(36, 36));
    m_overlayButton->setCursor(Qt::PointingHandCursor);
    m_overlayButton->setToolTip(tr("Play (Space)"));
    m_overlayButton->setAccessibleName(tr("Play"));
    m_overlayButton->setIcon(QIcon(renderSvgIcon(QStringLiteral(":/icons/play.svg"),
                                                 Qt::white, QSize(36, 36),
                                                 devicePixelRatioF())));

    m_playButton->setFixedSize(40, 36);
    m_positionSlider->setRange(0, 1000);
    m_positionSlider->setEnabled(false);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(80);
    m_volumeSlider->setMaximumWidth(120);
    m_volumeSlider->setToolTip(tr("Volume"));
    // Keep arrow keys free for the viewer's previous/next navigation.
    m_positionSlider->setFocusPolicy(Qt::NoFocus);
    m_volumeSlider->setFocusPolicy(Qt::NoFocus);
    m_statusLabel->setProperty("subheading", true);
    m_statusLabel->hide();

    auto *controls = new QHBoxLayout;
    controls->setSpacing(10);
    controls->addWidget(m_playButton);
    controls->addWidget(m_positionSlider, 1);
    controls->addWidget(m_timeLabel);
    auto *volumeLabel = new QLabel(tr("Volume"), this);
    volumeLabel->setProperty("subheading", true);
    controls->addWidget(volumeLabel);
    controls->addWidget(m_volumeSlider);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(m_zoomView, 1);
    layout->addWidget(m_statusLabel);
    layout->addLayout(controls);

    auto *playShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    playShortcut->setContext(Qt::WindowShortcut);
    connect(playShortcut, &QShortcut::activated, this, &VideoPlayerWidget::togglePlayback);

    connect(m_overlayButton, &QPushButton::clicked, this, &VideoPlayerWidget::play);
    connect(m_playButton, &QPushButton::clicked, this, &VideoPlayerWidget::togglePlayback);
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_audio->setVolume(value / 100.0f);
    });
    connect(m_positionSlider, &QSlider::sliderPressed, this, [this] {
        m_seeking = true;
    });
    connect(m_positionSlider, &QSlider::sliderReleased, this, [this] {
        if (m_player->duration() > 0) {
            m_player->setPosition(
                (m_positionSlider->value() * m_player->duration()) / 1000);
        }
        m_seeking = false;
    });
    connect(m_sink, &QVideoSink::videoFrameChanged, this,
            [this](const QVideoFrame &frame) {
                QVideoFrame copy(frame);
                const QImage image = copy.toImage();
                if (!image.isNull())
                    m_video->setFrame(image);
            });
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &VideoPlayerWidget::updatePosition);
    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        m_positionSlider->setEnabled(duration > 0);
        updatePosition(m_player->position());
    });
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this] {
        updatePlaybackButton();
        updateOverlay();
    });
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
                switch (status) {
                case QMediaPlayer::LoadingMedia:
                case QMediaPlayer::BufferingMedia:
                    m_statusLabel->setText(tr("Buffering…"));
                    m_statusLabel->show();
                    break;
                case QMediaPlayer::LoadedMedia:
                case QMediaPlayer::BufferedMedia:
                    m_statusLabel->hide();
                    break;
                case QMediaPlayer::EndOfMedia:
                    m_statusLabel->setText(tr("Playback finished"));
                    m_statusLabel->show();
                    break;
                default:
                    break;
                }
            });
    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &message) {
                m_statusLabel->setText(
                    message.isEmpty() ? tr("Video playback failed.") : message);
                m_statusLabel->show();
            });

    updatePlaybackButton();
}

void VideoPlayerWidget::setAsset(const ImmichAsset &asset, bool autoplay)
{
    stop();
    m_asset = asset;
    m_video->clearFrame();
    m_positionSlider->setValue(0);
    m_positionSlider->setEnabled(false);
    m_timeLabel->setText(QStringLiteral("0:00 / 0:00"));
    m_statusLabel->hide();
    m_playButton->setEnabled(true);
    updateOverlay();
    if (autoplay) {
        // Defer open so the window can paint first.
        QTimer::singleShot(0, this, [this, id = asset.id] {
            if (m_asset.id == id)
                play();
        });
    }
}

void VideoPlayerWidget::setPoster(const QPixmap &poster)
{
    if (m_player->playbackState() == QMediaPlayer::StoppedState && !poster.isNull())
        m_video->setFrame(poster.toImage());
}

void VideoPlayerWidget::play()
{
    if (!m_sourceSet) {
        const QUrl streamUrl = m_client->videoStreamUrl(m_asset.id);
        if (!streamUrl.isValid()) {
            m_statusLabel->setText(tr("Could not start the video stream."));
            m_statusLabel->show();
            m_playButton->setEnabled(false);
            return;
        }
        m_player->setSource(streamUrl);
        m_sourceSet = true;
    }
    m_player->play();
}

void VideoPlayerWidget::togglePlayback()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState)
        m_player->pause();
    else
        play();
}

void VideoPlayerWidget::stop()
{
    m_player->stop();
    m_player->setSource(QUrl());
    m_sourceSet = false;
}

void VideoPlayerWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateOverlay();
}

void VideoPlayerWidget::updatePlaybackButton()
{
    const bool playing = m_player->playbackState() == QMediaPlayer::PlayingState;
    configureActionButton(m_playButton, playing ? QStringLiteral(":/icons/pause.svg")
                                               : QStringLiteral(":/icons/play.svg"));
    m_playButton->setToolTip(playing ? tr("Pause (Space)") : tr("Play (Space)"));
    m_playButton->setAccessibleName(playing ? tr("Pause") : tr("Play"));
}

void VideoPlayerWidget::updateOverlay()
{
    m_overlayButton->move(m_zoomView->geometry().center() - m_overlayButton->rect().center());
    m_overlayButton->setVisible(m_player->playbackState() == QMediaPlayer::StoppedState);
    m_overlayButton->raise();
}

void VideoPlayerWidget::updatePosition(qint64 position)
{
    const qint64 duration = m_player->duration();
    if (!m_seeking && duration > 0) {
        m_positionSlider->setValue(
            static_cast<int>((position * 1000) / duration));
    }
    m_timeLabel->setText(
        QStringLiteral("%1 / %2").arg(formatTime(position), formatTime(duration)));
}

QString VideoPlayerWidget::formatTime(qint64 milliseconds)
{
    const qint64 totalSeconds = qMax<qint64>(0, milliseconds / 1000);
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;
    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2")
        .arg(minutes)
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

} // namespace Aurora
