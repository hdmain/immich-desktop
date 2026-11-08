#pragma once

#include "core/ImmichTypes.h"

#include <QPixmap>
#include <QWidget>

class QContextMenuEvent;
class QEnterEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPropertyAnimation;
class QResizeEvent;

namespace Aurora {

class VideoHoverPreview;

class MediaTile final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal selectionProgress READ selectionProgress WRITE setSelectionProgress)

public:
    explicit MediaTile(const ImmichAsset &asset, QWidget *parent = nullptr);

    const ImmichAsset &asset() const;
    qreal aspectRatio() const;
    bool hasThumbnail() const;
    int thumbnailDimension() const;
    // Trashed tiles offer Restore instead of Copy, Share and Move to trash.
    void setTrashed(bool trashed);
    bool hasThumbnailError() const;
    void setThumbnail(const QPixmap &thumbnail);
    void setThumbnailError(const QString &message);
    void clearThumbnailError();
    void clearThumbnail();
    void setTileSize(const QSize &size);
    void setHoverPreview(VideoHoverPreview *preview);
    void endHoverPreview();
    void updateAsset(const ImmichAsset &asset);
    // Selected tiles get a check and an accent border. While anything is selected
    // ("selecting"), every tile shows its check circle and a click selects.
    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }
    void setSelecting(bool selecting);
    qreal selectionProgress() const { return m_selectionProgress; }
    void setSelectionProgress(qreal progress);
    static void setSelectionColor(const QColor &color);
    static QColor selectionColor();

signals:
    void activated(const Aurora::ImmichAsset &asset);
    void highlighted(const Aurora::ImmichAsset &asset);
    void copyRequested(const Aurora::ImmichAsset &asset);
    void downloadRequested(const Aurora::ImmichAsset &asset);
    void shareRequested(const Aurora::ImmichAsset &asset);
    void infoRequested(const Aurora::ImmichAsset &asset);
    void restoreRequested(const Aurora::ImmichAsset &asset);
    void trashRequested(const Aurora::ImmichAsset &asset);
    void deleteRequested(const Aurora::ImmichAsset &asset);
    void favoriteRequested(const Aurora::ImmichAsset &asset, bool favorite);
    // A click meant for selecting: Ctrl/Shift held, the check circle, or any
    // click while selecting.
    void selectionClicked(const Aurora::ImmichAsset &asset, Qt::KeyboardModifiers modifiers);
    // Right-click on a selected tile: the menu acts on the whole selection.
    void selectionMenuRequested(const QPoint &globalPos);
    // An arrow key, Home or End: the grid moves focus (and with Shift, the selection).
    void navigationRequested(int key, Qt::KeyboardModifiers modifiers);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    static QString formatDuration(const QString &raw);
    QRect checkRect() const;
    QRect heartRect() const;
    bool selectable() const { return !m_asset.isLocalPending(); }
    // The heart corner is free: not trashed and no sync badge in its place.
    bool favoritable() const
    {
        return selectable() && !m_trashed && m_asset.localSyncState == LocalSyncState::None;
    }

    VideoHoverPreview *m_hoverPreview = nullptr;
    ImmichAsset m_asset;
    QPixmap m_thumbnail;
    bool m_trashed = false;
    QPoint m_pressPos;
    qreal m_resolvedAspectRatio;
    QString m_error;
    bool m_hasError = false;
    bool m_hoverPreviewActive = false;
    bool m_leftPressed = false;
    bool m_pressBecameDrag = false;
    bool m_pressOnCheck = false;
    bool m_pressOnHeart = false;
    bool m_hovered = false;
    bool m_selected = false;
    bool m_selecting = false;
    qreal m_selectionProgress = 0.0;
    QPropertyAnimation *m_selectionAnimation = nullptr;
};

} // namespace Aurora
