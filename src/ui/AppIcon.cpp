#include "ui/AppIcon.h"

#include "core/Theme.h"
#include "ui/IconUtils.h"

#include <QPainter>
#include <QPixmap>

namespace Aurora {
namespace {

QIcon iconFromPrefix(const QString &prefix)
{
    QIcon icon;
    const int sizes[] = {16, 32, 48, 64, 128, 256};
    for (int size : sizes) {
        const QString path = prefix.isEmpty()
                                 ? QStringLiteral(":/branding/immich-logo-%1.png").arg(size)
                                 : QStringLiteral(":/branding/immich-logo-%1-%2.png")
                                       .arg(prefix, QString::number(size));
        const QPixmap pixmap(path);
        if (!pixmap.isNull())
            icon.addPixmap(pixmap);
    }

    if (icon.isNull()) {
        const QString fallback = prefix.isEmpty()
                                     ? QStringLiteral(":/branding/immich-logo.png")
                                     : QStringLiteral(":/branding/immich-logo-%1.png").arg(prefix);
        icon.addFile(fallback);
    }
    return icon;
}

QIcon transferArrowIcon(const QString &svgPath, const ThemePalette &palette)
{
    const bool dark = isDarkPalette(palette);
    const QColor foreground = dark ? QColor(QStringLiteral("#F5F7FB"))
                                   : QColor(QStringLiteral("#172033"));
    const QColor background = dark ? QColor(QStringLiteral("#151B26"))
                                   : QColor(QStringLiteral("#FFFFFF"));
    const QColor accent = palette.accent.isValid() ? palette.accent
                                                   : QColor(QStringLiteral("#2B79C3"));

    QIcon icon;
    for (int size : {16, 20, 24, 32, 48, 64}) {
        QPixmap canvas(size, size);
        canvas.fill(Qt::transparent);
        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(background);
        painter.drawRoundedRect(QRectF(0.5, 0.5, size - 1.0, size - 1.0), size * 0.22,
                                size * 0.22);
        painter.setBrush(accent);
        const qreal pad = size * 0.12;
        painter.drawRoundedRect(QRectF(pad, pad, size - 2 * pad, size - 2 * pad), size * 0.18,
                                size * 0.18);

        const int glyph = qMax(10, qRound(size * 0.55));
        const QPixmap arrow =
            renderSvgIcon(svgPath, foreground, QSize(glyph, glyph), 1.0);
        const int x = (size - glyph) / 2;
        const int y = (size - glyph) / 2;
        painter.drawPixmap(x, y, arrow);
        painter.end();
        icon.addPixmap(canvas);
    }
    return icon;
}

} // namespace

QIcon applicationIcon()
{
    return iconFromPrefix({});
}

QIcon applicationIconForPalette(const ThemePalette &palette)
{
    if (isDarkPalette(palette))
        return iconFromPrefix({});
    return iconFromPrefix(QStringLiteral("on-light"));
}

QIcon trayUploadIcon(const ThemePalette &palette)
{
    return transferArrowIcon(QStringLiteral(":/icons/upload.svg"), palette);
}

QIcon trayDownloadIcon(const ThemePalette &palette)
{
    return transferArrowIcon(QStringLiteral(":/icons/download.svg"), palette);
}

} // namespace Aurora
