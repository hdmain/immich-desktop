#pragma once

#include <QPixmap>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;

namespace Aurora {

// A cover-photo tile with a label and count overlay, used for the Year grid
// and the day tiles in the Year-detail view. Deliberately simpler than
// MediaTile: no selection, pinning, upload-pending state, or context menu -
// just "here's a summary of a group of photos, click it to drill in."
class SummaryTile final : public QWidget {
    Q_OBJECT

public:
    explicit SummaryTile(const QString &label, int count, QWidget *parent = nullptr);

    void setThumbnail(const QPixmap &thumbnail);
    void setTileSize(const QSize &size);
    void setShowCount(bool show);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QString m_label;
    int m_count;
    bool m_showCount = true;
    QPixmap m_thumbnail;
};

} // namespace Aurora
