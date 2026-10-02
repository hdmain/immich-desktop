#pragma once

#include "core/ImmichClient.h"

#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QWidget>

class QThread;
class QTimer;

namespace Aurora {

class MediaTile;
class SoftwareVideoWidget;
class HoverPreviewEngine;

class VideoHoverPreview final : public QObject {
    Q_OBJECT

public:
    explicit VideoHoverPreview(ImmichClient *client, QWidget *hostWidget,
                               QObject *parent = nullptr);
    ~VideoHoverPreview() override;

    void showForTile(MediaTile *tile);
    void hideForTile(MediaTile *tile);
    void updateTileGeometry(MediaTile *tile);
    void stop();
    void setEnabled(bool enabled);

private:
    void armStart(MediaTile *tile, const QUrl &streamUrl);
    void beginPlayback(const QUrl &streamUrl);
    void scheduleStop();
    void detachOverlay();
    void teardownWorker();

    ImmichClient *m_client = nullptr;
    QWidget *m_hostWidget = nullptr;
    QPointer<MediaTile> m_activeTile;
    QPointer<MediaTile> m_pendingTile;
    SoftwareVideoWidget *m_overlay = nullptr;
    QTimer *m_startTimer = nullptr;
    QTimer *m_stopTimer = nullptr;
    QThread *m_thread = nullptr;
    HoverPreviewEngine *m_engine = nullptr;
    QUrl m_pendingUrl;
    QUrl m_loadedUrl;
    bool m_enabled = true;
};

} // namespace Aurora
