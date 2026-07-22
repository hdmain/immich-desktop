#pragma once

#include "core/ImmichClient.h"

#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QWidget>

class QAudioOutput;
class QMediaPlayer;
class QTimer;
class QVideoWidget;

namespace Aurora {

class MediaTile;

class VideoHoverPreview final : public QObject {
    Q_OBJECT

public:
    explicit VideoHoverPreview(ImmichClient *client, QWidget *hostWidget,
                               QObject *parent = nullptr);

    void showForTile(MediaTile *tile);
    void hideForTile(MediaTile *tile);
    void updateTileGeometry(MediaTile *tile);
    void stop();

private:
    void beginPlayback(const QUrl &streamUrl);
    void scheduleStop();

    ImmichClient *m_client = nullptr;
    QWidget *m_hostWidget = nullptr;
    QPointer<MediaTile> m_activeTile;
    QVideoWidget *m_video = nullptr;
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audio = nullptr;
    QTimer *m_stopTimer = nullptr;
    bool m_handlingPlayer = false;
};

} // namespace Aurora
