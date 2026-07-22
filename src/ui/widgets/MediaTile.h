#pragma once

#include "core/ImmichTypes.h"

#include <QPixmap>
#include <QWidget>

class QContextMenuEvent;
class QEnterEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;

namespace Aurora {

class VideoHoverPreview;

class MediaTile final : public QWidget {
    Q_OBJECT

public:
    explicit MediaTile(const ImmichAsset &asset, QWidget *parent = nullptr);

    const ImmichAsset &asset() const;
    qreal aspectRatio() const;
    bool hasThumbnail() const;
    bool hasThumbnailError() const;
    void setThumbnail(const QPixmap &thumbnail);
    void setThumbnailError(const QString &message);
    void clearThumbnail();
    void setTileSize(const QSize &size);
    void setHoverPreview(VideoHoverPreview *preview);
    void endHoverPreview();

signals:
    void activated(const Aurora::ImmichAsset &asset);
    void highlighted(const Aurora::ImmichAsset &asset);
    void copyRequested(const Aurora::ImmichAsset &asset);
    void downloadRequested(const Aurora::ImmichAsset &asset);
    void trashRequested(const Aurora::ImmichAsset &asset);
    void deleteRequested(const Aurora::ImmichAsset &asset);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    static QString formatDuration(const QString &raw);

    VideoHoverPreview *m_hoverPreview = nullptr;
    ImmichAsset m_asset;
    QPixmap m_thumbnail;
    qreal m_resolvedAspectRatio;
    QString m_error;
    bool m_hasError = false;
    bool m_hoverPreviewActive = false;
};

} // namespace Aurora
