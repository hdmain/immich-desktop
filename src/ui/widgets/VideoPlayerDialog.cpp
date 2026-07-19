#include "ui/widgets/VideoPlayerDialog.h"

#include <QAudioOutput>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaPlayer>
#include <QPushButton>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QVideoWidget>

namespace Aurora {

VideoPlayerDialog::VideoPlayerDialog(ImmichClient *client, const ImmichAsset &asset,
                                     QWidget *parent)
    : QDialog(parent)
    , m_asset(asset)
    , m_player(new QMediaPlayer(this))
    , m_audio(new QAudioOutput(this))
    , m_video(new QVideoWidget(this))
    , m_playButton(new QPushButton(this))
    , m_downloadButton(new QPushButton(tr("Download"), this))
    , m_trashButton(new QPushButton(tr("Trash"), this))
    , m_deleteButton(new QPushButton(tr("Delete"), this))
    , m_positionSlider(new QSlider(Qt::Horizontal, this))
    , m_volumeSlider(new QSlider(Qt::Horizontal, this))
    , m_timeLabel(new QLabel(QStringLiteral("0:00 / 0:00"), this))
    , m_statusLabel(new QLabel(tr("Opening stream…"), this))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(asset.fileName.isEmpty() ? tr("Video") : asset.fileName);
    resize(1000, 700);

    m_video->setMinimumSize(640, 360);
    m_video->setStyleSheet(QStringLiteral("background: black;"));
    m_player->setAudioOutput(m_audio);
    m_player->setVideoOutput(m_video);
    m_audio->setVolume(0.8f);

    m_playButton->setFixedSize(40, 36);
    m_playButton->setToolTip(tr("Play"));
    m_downloadButton->setToolTip(tr("Download original file"));
    m_trashButton->setToolTip(tr("Move to trash"));
    m_deleteButton->setToolTip(tr("Delete permanently"));
    m_positionSlider->setRange(0, 1000);
    m_positionSlider->setEnabled(false);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(80);
    m_volumeSlider->setMaximumWidth(120);
    m_volumeSlider->setToolTip(tr("Volume"));
    m_statusLabel->setProperty("subheading", true);

    auto *controls = new QHBoxLayout;
    controls->setSpacing(10);
    controls->addWidget(m_playButton);
    controls->addWidget(m_positionSlider, 1);
    controls->addWidget(m_timeLabel);
    auto *volumeLabel = new QLabel(tr("Volume"), this);
    volumeLabel->setProperty("subheading", true);
    controls->addWidget(volumeLabel);
    controls->addWidget(m_volumeSlider);
    controls->addWidget(m_downloadButton);
    controls->addWidget(m_trashButton);
    controls->addWidget(m_deleteButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    layout->addWidget(m_video, 1);
    layout->addWidget(m_statusLabel);
    layout->addLayout(controls);

    connect(m_playButton, &QPushButton::clicked,
            this, &VideoPlayerDialog::togglePlayback);
    connect(m_downloadButton, &QPushButton::clicked, this, [this] {
        emit downloadRequested(m_asset);
    });
    connect(m_trashButton, &QPushButton::clicked, this, [this] {
        emit trashRequested(m_asset);
    });
    connect(m_deleteButton, &QPushButton::clicked, this, [this] {
        emit deleteRequested(m_asset);
    });
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
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &VideoPlayerDialog::updatePosition);
    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        m_positionSlider->setEnabled(duration > 0);
        updatePosition(m_player->position());
    });
    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, [this] { updatePlaybackButton(); });
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

    const QUrl streamUrl = client->videoStreamUrl(asset.id);
    if (!streamUrl.isValid()) {
        m_statusLabel->setText(tr("Could not start the video stream."));
        m_playButton->setEnabled(false);
    } else {
        m_player->setSource(streamUrl);
        QTimer::singleShot(0, m_player, &QMediaPlayer::play);
    }
    updatePlaybackButton();
}

void VideoPlayerDialog::togglePlayback()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState)
        m_player->pause();
    else
        m_player->play();
}

void VideoPlayerDialog::updatePlaybackButton()
{
    const bool playing = m_player->playbackState() == QMediaPlayer::PlayingState;
    m_playButton->setIcon(style()->standardIcon(
        playing ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
    m_playButton->setToolTip(playing ? tr("Pause") : tr("Play"));
}

void VideoPlayerDialog::updatePosition(qint64 position)
{
    const qint64 duration = m_player->duration();
    if (!m_seeking && duration > 0) {
        m_positionSlider->setValue(
            static_cast<int>((position * 1000) / duration));
    }
    m_timeLabel->setText(
        QStringLiteral("%1 / %2").arg(formatTime(position), formatTime(duration)));
}

QString VideoPlayerDialog::formatTime(qint64 milliseconds)
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
