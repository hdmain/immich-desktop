#include "ui/widgets/Sidebar.h"

#include "AppVersion.h"
#include "core/AppSettings.h"
#include "core/ThemeManager.h"
#include "ui/widgets/AnimatedButton.h"
#include "ui/widgets/BrowseTree.h"

#include <QButtonGroup>
#include <QEasingCurve>
#include <QLabel>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariantAnimation>

namespace Aurora {
namespace {

// Button-group ids for browse sections, kept clear of the page indices 0-6.
constexpr int kSectionIdBase = 10;

int sectionId(BrowseSection section)
{
    return kSectionIdBase + static_cast<int>(section);
}

} // namespace

Sidebar::Sidebar(ThemeManager *themeManager, QWidget *parent)
    : QWidget(parent)
    , m_themeManager(themeManager)
    , m_navigationLabel(new QLabel(this))
    , m_navigationLayout(new QVBoxLayout)
    , m_navigationGroup(nullptr)
    , m_version(new QLabel(this))
    , m_toggleButton(new AnimatedButton(tr("Collapse"), QStringLiteral(":/icons/panel-left-close.svg"),
                                        themeManager, this))
    , m_widthAnimation(new QVariantAnimation(this))
    , m_collapsed(AppSettings().loadWindow().sidebarCollapsed)
{
    setObjectName(QStringLiteral("sidebar"));

    for (BrowseSection section :
         {BrowseSection::Albums, BrowseSection::Tags, BrowseSection::Folders}) {
        auto *tree = new BrowseTree(section, this);
        tree->hide();
        m_trees.insert(section, tree);
        m_treeExpanded.insert(section, false);
        // Wait for the sidebar to lay out the newly shown or resized list.
        connect(tree, &BrowseTree::itemSelected, this, [this, tree] {
            QTimer::singleShot(0, this, [this, tree] {
                const QRect rect = tree->selectedRect();
                if (!tree->isVisible() || rect.isEmpty())
                    return;
                const QPoint center =
                    tree->viewport()->mapTo(m_scroll->widget(), rect.center());
                m_scroll->ensureVisible(center.x(), center.y(), 0, rect.height());
            });
        });
    }

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 18, 14, 14);
    layout->setSpacing(8);

    m_navigationLabel->setProperty("subheading", true);
    m_navigationLabel->setContentsMargins(8, 4, 0, 8);
    layout->addWidget(m_navigationLabel);

    auto *navigation = new QWidget;
    navigation->setObjectName(QStringLiteral("sidebarNavigation"));
    auto *navigationColumn = new QVBoxLayout(navigation);
    navigationColumn->setContentsMargins(0, 0, 0, 0);
    m_navigationLayout->setContentsMargins(0, 0, 0, 0);
    m_navigationLayout->setSpacing(8);
    navigationColumn->addLayout(m_navigationLayout);
    navigationColumn->addStretch();
    auto *scroll = new QScrollArea(this);
    m_scroll = scroll;
    scroll->setObjectName(QStringLiteral("sidebarScroll"));
    scroll->setWidget(navigation);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(scroll, 1);
    showMainNavigation();

    m_toggleButton->setCheckable(false);
    connect(m_toggleButton, &QPushButton::clicked, this, &Sidebar::toggleCollapsed);
    layout->addWidget(m_toggleButton);

    m_version->setText(tr("fan-made\nimmich desktop  ·  %1").arg(Config::displayVersion()));
    m_version->setProperty("subheading", true);
    m_version->setAlignment(Qt::AlignCenter);
    m_version->setWordWrap(true);
    layout->addWidget(m_version);

    m_widthAnimation->setDuration(160);
    m_widthAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_widthAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &width) { setFixedWidth(width.toInt()); });
    connect(m_themeManager, &ThemeManager::appearanceChanged, this,
            [this] { refreshLayout(false); });
    refreshLayout(false);
}

BrowseTree *Sidebar::browseTree(BrowseSection section) const
{
    return m_trees.value(section);
}

bool Sidebar::isSectionExpanded(BrowseSection section) const
{
    return m_treeExpanded.value(section);
}

void Sidebar::updateTreeVisibility()
{
    const bool collapsed = m_themeManager->minimalistUi() || isCollapsed();
    for (auto it = m_trees.cbegin(); it != m_trees.cend(); ++it)
        it.value()->setVisible(!m_settingsMode && !collapsed && m_treeExpanded.value(it.key()));
}

void Sidebar::toggleCollapsed()
{
    if (m_narrow) {
        m_expandedWhileNarrow = !m_expandedWhileNarrow;
    } else {
        m_collapsed = !m_collapsed;
        WindowSettings window = AppSettings().loadWindow();
        window.sidebarCollapsed = m_collapsed;
        AppSettings().saveWindow(window);
    }
    refreshLayout(true);
}

void Sidebar::setNarrow(bool narrow)
{
    if (m_narrow == narrow)
        return;
    m_narrow = narrow;
    m_expandedWhileNarrow = false;
    refreshLayout(true);
}

bool Sidebar::isCollapsed() const
{
    return m_narrow ? !m_expandedWhileNarrow : m_collapsed;
}

void Sidebar::refreshLayout(bool animate)
{
    const bool minimal = m_themeManager->minimalistUi();
    const bool collapsed = minimal || isCollapsed();
    m_navigationLabel->setVisible(!collapsed);
    m_version->setVisible(!collapsed);
    m_toggleButton->setVisible(!minimal);
    m_toggleButton->setText(collapsed ? tr("Expand") : tr("Collapse"));
    m_toggleButton->setToolTip(collapsed ? tr("Expand sidebar (F9)")
                                         : tr("Collapse sidebar (F9)"));
    m_toggleButton->setIconResource(collapsed ? QStringLiteral(":/icons/panel-left-open.svg")
                                              : QStringLiteral(":/icons/panel-left-close.svg"));
    for (AnimatedButton *button : findChildren<AnimatedButton *>())
        button->setIconOnly(collapsed);
    updateTreeVisibility();

    const int target = collapsed ? 76 : 224;
    m_widthAnimation->stop();
    if (animate && isVisible() && width() != target) {
        m_widthAnimation->setStartValue(width());
        m_widthAnimation->setEndValue(target);
        m_widthAnimation->start();
    } else {
        setFixedWidth(target);
    }
}

void Sidebar::setCurrentPage(int index)
{
    if (index <= 1)
        showMainNavigation();
    else
        showSettingsNavigation();

    for (BrowseTree *tree : m_trees)
        tree->selectValue({});
    if (auto *button = m_navigationGroup->button(index))
        button->setChecked(true);
}

void Sidebar::setActiveSection(BrowseSection section, const QString &itemValue)
{
    showMainNavigation();
    // An album, tag or folder opened from elsewhere (e.g. the info panel) should
    // show up in its list, not just light up the section button.
    if (!itemValue.isEmpty())
        expandSection(section);
    for (auto it = m_trees.cbegin(); it != m_trees.cend(); ++it)
        it.value()->selectValue(it.key() == section ? itemValue : QString());
    if (auto *button = m_navigationGroup->button(sectionId(section)))
        button->setChecked(true);
}

void Sidebar::expandSection(BrowseSection section)
{
    auto *button = qobject_cast<AnimatedButton *>(m_navigationGroup->button(sectionId(section)));
    if (!m_trees.contains(section) || m_treeExpanded.value(section))
        return;
    if (button)
        button->setExpanded(true);
    m_treeExpanded.insert(section, true);
    updateTreeVisibility();
    emit sectionExpanded(section);
}

AnimatedButton *Sidebar::addNavigation(const QString &label, const QString &icon, int index,
                                       bool checkable)
{
    auto *button = new AnimatedButton(label, icon, m_themeManager, this);
    button->setCheckable(checkable);
    button->setIconOnly(m_themeManager->minimalistUi() || isCollapsed());
    if (checkable)
        m_navigationGroup->addButton(button, index);
    m_navigationLayout->addWidget(button);
    if (index < kSectionIdBase) {
        connect(button, &QPushButton::clicked, this, [this, index] {
            emit pageRequested(index);
        });
    }
    return button;
}

void Sidebar::addSectionNavigation(BrowseSection section, const QString &label,
                                   const QString &icon)
{
    AnimatedButton *button = addNavigation(label, icon, sectionId(section));
    // Opening a section also unfolds its list; only the chevron folds it again.
    connect(button, &QPushButton::clicked, this, [this, section] {
        expandSection(section);
        emit sectionRequested(section);
    });
    BrowseTree *tree = m_trees.value(section);
    if (!tree)
        return;
    button->setExpandable(true);
    button->setExpanded(m_treeExpanded.value(section));
    connect(button, &AnimatedButton::expandToggled, this, [this, section](bool expanded) {
        m_treeExpanded.insert(section, expanded);
        updateTreeVisibility();
        if (expanded)
            emit sectionExpanded(section);
    });
    m_navigationLayout->addWidget(tree);
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
            // Trees keep their loaded albums, tags and folders between rebuilds.
            if (!qobject_cast<BrowseTree *>(widget))
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
    m_navigationLabel->setText(tr("NAVIGATION"));
    clearNavigation();
    addNavigation(tr("Library"),
                  QStringLiteral(":/icons/images.svg"), 0);
    addNavigation(tr("Explore"),
                  QStringLiteral(":/icons/layout-dashboard.svg"), 1);
    addSectionNavigation(BrowseSection::People, tr("People"), QStringLiteral(":/icons/users.svg"));
    addSectionNavigation(BrowseSection::Favorites, tr("Favorites"),
                         QStringLiteral(":/icons/heart.svg"));
    addSectionNavigation(BrowseSection::Archive, tr("Archive"),
                         QStringLiteral(":/icons/archive.svg"));
    addSectionNavigation(BrowseSection::Albums, tr("Albums"),
                         QStringLiteral(":/icons/book-image.svg"));
    addSectionNavigation(BrowseSection::Tags, tr("Tags"), QStringLiteral(":/icons/tag.svg"));
    addSectionNavigation(BrowseSection::Folders, tr("Folders"),
                         QStringLiteral(":/icons/folder.svg"));
    addSectionNavigation(BrowseSection::Trash, tr("Trash"), QStringLiteral(":/icons/trash.svg"));
    addNavigation(tr("Settings"),
                  QStringLiteral(":/icons/settings.svg"), 2);
    if (auto *libraryButton = m_navigationGroup->button(0))
        libraryButton->setChecked(true);
    updateTreeVisibility();
}

void Sidebar::showSettingsNavigation()
{
    if (m_settingsMode && m_navigationGroup)
        return;

    m_settingsMode = true;
    m_navigationLabel->setText(tr("SETTINGS"));
    clearNavigation();
    addNavigation(tr("Immich Server"),
                  QStringLiteral(":/icons/server.svg"), 2);
    addNavigation(tr("Folder Sync"),
                  QStringLiteral(":/icons/folder-sync.svg"), 3);
    addNavigation(tr("Appearance"),
                  QStringLiteral(":/icons/palette.svg"), 4);
    addNavigation(tr("Update"),
                  QStringLiteral(":/icons/refresh-cw.svg"), 5);
    addNavigation(tr("About"),
                  QStringLiteral(":/icons/star.svg"), 6);
    addNavigation(tr("Back"),
                  QStringLiteral(":/icons/arrow-left.svg"), 0, false);
    updateTreeVisibility();
}

} // namespace Aurora
