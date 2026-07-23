#include "ui/AppIcon.h"

#include "core/Theme.h"

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

} // namespace

QIcon applicationIcon()
{
    return iconFromPrefix({});
}

QIcon applicationIconForPalette(const ThemePalette &palette)
{
    // Dark UI -> white mark (default set). Light UI -> dark mark.
    if (isDarkPalette(palette))
        return iconFromPrefix({});
    return iconFromPrefix(QStringLiteral("on-light"));
}

} // namespace Aurora
