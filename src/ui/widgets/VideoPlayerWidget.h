#pragma once

#include "core/ImmichClient.h"

#include <QWidget>

class QAudioOutput;
class QLabel;
class QMediaPlayer;
class QPushButton;
class QSlider;
class QVideoSink;

namespace Aurora {

class SoftwareVideoWidget;
class ZoomPanWidget;

class VideoPlayerWidget final : public QWidget {
    Q_OBJECT

public:
    explicit VideoPlayerWidget(ImmichClient *client, QWidget *parent = nullptr);

    void setAsset(const ImmichAsset &asset, bool autoplay);
    void setPoster(const QPixmap &poster);
    void togglePlayback();
    void stop();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void play();
    void updatePlaybackButton();
    void updatePosition(qint64 position);
    void updateOverlay();
    static QString formatTime(qint64 milliseconds);

    ImmichClient *m_client;
    ImmichAsset m_asset;
    QMediaPlayer *m_player;
    QAudioOutput *m_audio;
    QVideoSink *m_sink;
    SoftwareVideoWidget *m_video;
    ZoomPanWidget *m_zoomView;
    QPushButton *m_overlayButton;
    QPushButton *m_playButton;
    QSlider *m_positionSlider;
    QSlider *m_volumeSlider;
    QLabel *m_timeLabel;
    QLabel *m_statusLabel;
    bool m_seeking = false;
    bool m_sourceSet = false;
};

} // namespace Aurora
