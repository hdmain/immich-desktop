#pragma once

#include "core/ImmichClient.h"

#include <QPixmap>
#include <QWidget>

class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

namespace Aurora {

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

signals:
    void activated(const Aurora::ImmichAsset &asset);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    static QString formatDuration(const QString &raw);

    ImmichAsset m_asset;
    QPixmap m_thumbnail;
    qreal m_resolvedAspectRatio;
    QString m_error;
    bool m_hasError = false;
};

} // namespace Aurora
