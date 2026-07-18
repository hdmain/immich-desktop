#include "ui/AppIcon.h"

#include <QPixmap>

namespace Aurora {

QIcon applicationIcon()
{
    QIcon icon;
    const int sizes[] = {16, 32, 48, 64, 128, 256};
    for (int size : sizes) {
        const QString path =
            QStringLiteral(":/branding/immich-logo-%1.png").arg(size);
        const QPixmap pixmap(path);
        if (!pixmap.isNull())
            icon.addPixmap(pixmap);
    }

    if (icon.isNull())
        icon.addFile(QStringLiteral(":/branding/immich-logo.png"));
    return icon;
}

} // namespace Aurora
