#include "ui/IconUtils.h"

#include <QFile>
#include <QPainter>
#include <QSvgRenderer>

namespace Aurora {

QPixmap renderSvgIcon(const QString &resourcePath, const QColor &color,
                      const QSize &size, qreal devicePixelRatio)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    QByteArray source = file.readAll();
    source.replace("currentColor", color.name(QColor::HexRgb).toUtf8());
    QSvgRenderer renderer(source);
    if (!renderer.isValid())
        return {};

    const QSize pixelSize = (size * devicePixelRatio).expandedTo(QSize(1, 1));
    QPixmap pixmap(pixelSize);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(devicePixelRatio);

    QPainter painter(&pixmap);
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));
    return pixmap;
}

} // namespace Aurora
