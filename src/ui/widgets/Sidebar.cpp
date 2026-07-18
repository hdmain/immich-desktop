#include "ui/widgets/Sidebar.h"

#include "AppVersion.h"
#include "core/ThemeManager.h"
#include "ui/widgets/AnimatedButton.h"

#include <QButtonGroup>
#include <QLabel>
#include <QVBoxLayout>

namespace Aurora {

Sidebar::Sidebar(ThemeManager *themeManager, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("sidebar"));
    setFixedWidth(224);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 18, 14, 14);
    layout->setSpacing(8);

    auto *navigationLabel = new QLabel(QStringLiteral("NAVIGATION"), this);
    navigationLabel->setProperty("subheading", true);
    navigationLabel->setContentsMargins(8, 4, 0, 8);
    layout->addWidget(navigationLabel);

    auto *group = new QButtonGroup(this);
    group->setExclusive(true);

    auto addNavigation = [this, layout, group, themeManager](
                             const QString &label, const QString &icon, int index) {
        auto *button = new AnimatedButton(label, icon, themeManager, this);
        group->addButton(button, index);
        layout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, index] {
            emit pageRequested(index);
        });
        return button;
    };

    auto *dashboard = addNavigation(
        QStringLiteral("Overview"), QStringLiteral(":/icons/layout-dashboard.svg"), 0);
    addNavigation(QStringLiteral("Appearance"), QStringLiteral(":/icons/palette.svg"), 1);
    addNavigation(QStringLiteral("Updates"), QStringLiteral(":/icons/refresh-cw.svg"), 2);
    dashboard->setChecked(true);

    layout->addStretch();

    auto *version = new QLabel(
        tr("fan-made\nimmich desktop  ·  v%1")
            .arg(QString::fromLatin1(Config::ApplicationVersion)),
        this);
    version->setProperty("subheading", true);
    version->setAlignment(Qt::AlignCenter);
    version->setWordWrap(true);
    layout->addWidget(version);
}

} // namespace Aurora
