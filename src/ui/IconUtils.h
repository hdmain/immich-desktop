#pragma once

#include <QColor>
#include <QPixmap>
#include <QSize>
#include <QString>

namespace Aurora {

QPixmap renderSvgIcon(const QString &resourcePath, const QColor &color,
                      const QSize &size, qreal devicePixelRatio = 1.0);

} // namespace Aurora
