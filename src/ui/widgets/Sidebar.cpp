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
    , m_themeManager(themeManager)
    , m_navigationLabel(new QLabel(this))
    , m_navigationLayout(new QVBoxLayout)
    , m_navigationGroup(nullptr)
{
    setObjectName(QStringLiteral("sidebar"));
    setFixedWidth(224);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 18, 14, 14);
    layout->setSpacing(8);

    m_navigationLabel->setProperty("subheading", true);
    m_navigationLabel->setContentsMargins(8, 4, 0, 8);
    layout->addWidget(m_navigationLabel);
    m_navigationLayout->setContentsMargins(0, 0, 0, 0);
    m_navigationLayout->setSpacing(8);
    layout->addLayout(m_navigationLayout);
    showMainNavigation();

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

void Sidebar::setCurrentPage(int index)
{
    if (index == 0)
        showMainNavigation();
    else
        showSettingsNavigation();

    if (auto *button = m_navigationGroup->button(index))
        button->setChecked(true);
}

void Sidebar::addNavigation(const QString &label, const QString &icon, int index,
                            bool checkable)
{
    auto *button = new AnimatedButton(label, icon, m_themeManager, this);
    button->setCheckable(checkable);
    if (checkable)
        m_navigationGroup->addButton(button, index);
    m_navigationLayout->addWidget(button);
    connect(button, &QPushButton::clicked, this, [this, index] {
        emit pageRequested(index);
    });
}

void Sidebar::clearNavigation()
{
    if (m_navigationGroup) {
        m_navigationGroup->setExclusive(false);
        m_navigationGroup->deleteLater();
    }
    m_navigationGroup = new QButtonGroup(this);
    m_navigationGroup->setExclusive(true);

    while (auto *item = m_navigationLayout->takeAt(0)) {
        if (auto *widget = item->widget()) {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }
}

void Sidebar::showMainNavigation()
{
    if (!m_settingsMode && m_navigationGroup)
        return;

    m_settingsMode = false;
    m_navigationLabel->setText(QStringLiteral("NAVIGATION"));
    clearNavigation();
    addNavigation(QStringLiteral("Library"),
                  QStringLiteral(":/icons/images.svg"), 0);
    addNavigation(QStringLiteral("Settings"),
                  QStringLiteral(":/icons/settings.svg"), 1);
    if (auto *libraryButton = m_navigationGroup->button(0))
        libraryButton->setChecked(true);
}

void Sidebar::showSettingsNavigation()
{
    if (m_settingsMode && m_navigationGroup)
        return;

    m_settingsMode = true;
    m_navigationLabel->setText(QStringLiteral("SETTINGS"));
    clearNavigation();
    addNavigation(QStringLiteral("Immich Server"),
                  QStringLiteral(":/icons/server.svg"), 1);
    addNavigation(QStringLiteral("Appearance"),
                  QStringLiteral(":/icons/palette.svg"), 2);
    addNavigation(QStringLiteral("Update"),
                  QStringLiteral(":/icons/refresh-cw.svg"), 3);
    addNavigation(QStringLiteral("Back"),
                  QStringLiteral(":/icons/arrow-left.svg"), 0, false);
}

} // namespace Aurora
