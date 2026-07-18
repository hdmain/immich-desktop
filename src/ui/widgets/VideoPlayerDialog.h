#pragma once

#include "core/ImmichClient.h"

#include <QDialog>

class QAudioOutput;
class QLabel;
class QMediaPlayer;
class QPushButton;
class QSlider;
class QVideoWidget;

namespace Aurora {

class VideoPlayerDialog final : public QDialog {
    Q_OBJECT

public:
    explicit VideoPlayerDialog(ImmichClient *client, const ImmichAsset &asset,
                               QWidget *parent = nullptr);

private:
    void togglePlayback();
    void updatePlaybackButton();
    void updatePosition(qint64 position);
    static QString formatTime(qint64 milliseconds);

    QMediaPlayer *m_player;
    QAudioOutput *m_audio;
    QVideoWidget *m_video;
    QPushButton *m_playButton;
    QSlider *m_positionSlider;
    QSlider *m_volumeSlider;
    QLabel *m_timeLabel;
    QLabel *m_statusLabel;
    bool m_seeking = false;
};

} // namespace Aurora
