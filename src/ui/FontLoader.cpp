#include "ui/FontLoader.h"

#include <QFontDatabase>

namespace Aurora {

QStringList loadApplicationFonts()
{
    const QStringList resources = {
        QStringLiteral(":/fonts/Inter-Regular.ttf"),
        QStringLiteral(":/fonts/Inter-Medium.ttf"),
        QStringLiteral(":/fonts/Inter-SemiBold.ttf"),
        QStringLiteral(":/fonts/Inter-Bold.ttf")
    };

    QStringList families;
    for (const QString &path : resources) {
        const int id = QFontDatabase::addApplicationFont(path);
        if (id < 0)
            continue;
        families.append(QFontDatabase::applicationFontFamilies(id));
    }
    families.removeDuplicates();
    return families;
}

} // namespace Aurora
