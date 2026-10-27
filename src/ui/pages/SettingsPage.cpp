#include "ui/pages/SettingsPage.h"

#include "core/ThemeManager.h"
#include "ui/pages/AboutPage.h"
#include "ui/pages/AppearancePage.h"
#include "ui/pages/ConnectionPage.h"
#include "ui/pages/SyncFolderPage.h"
#include "ui/pages/UpdatesPage.h"
#include "ui/IconUtils.h"
#include "ui/widgets/AnimatedStackedWidget.h"

#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>

namespace Aurora {

SettingsPage::SettingsPage(ThemeManager *themeManager, UpdateManager *updateManager,
                           ImmichClient *immichClient, FolderSyncService *folderSync,
                           QWidget *parent)
    : QWidget(parent)
    , m_sections(new AnimatedStackedWidget(this))
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(4, 6, 4, 4);
    root->setSpacing(8);

    auto *headerRow = new QHBoxLayout;
    headerRow->setSpacing(14);
    auto *backButton = new QPushButton(tr("Back"), this);
    configureActionButton(backButton, QStringLiteral(":/icons/arrow-left.svg"));
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setToolTip(tr("Back (Esc)"));
    connect(backButton, &QPushButton::clicked, this, &SettingsPage::backRequested);
    auto *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape, &QShortcut::activated, this, &SettingsPage::backRequested);

    auto *titles = new QVBoxLayout;
    titles->setSpacing(2);
    auto *heading = new QLabel(tr("Settings"), this);
    heading->setProperty("heading", true);
    auto *subheading = new QLabel(
        tr("Choose a settings section from the sidebar."), this);
    subheading->setProperty("subheading", true);
    titles->addWidget(heading);
    titles->addWidget(subheading);
    headerRow->addWidget(backButton, 0, Qt::AlignVCenter);
    headerRow->addLayout(titles, 1);
    root->addLayout(headerRow);

    auto *connection = new ConnectionPage(immichClient, m_sections);
    connect(connection, &ConnectionPage::profilesChanged, this, &SettingsPage::profilesChanged);
    m_sections->addWidget(connection);
    m_sections->addWidget(new SyncFolderPage(folderSync, themeManager, m_sections));
    auto *appearance = new AppearancePage(themeManager, m_sections);
    connect(appearance, &AppearancePage::languageChangeRequested,
            this, &SettingsPage::languageChangeRequested);
    m_sections->addWidget(appearance);
    m_sections->addWidget(new UpdatesPage(updateManager, m_sections));
    m_sections->addWidget(new AboutPage(m_sections));
    root->addWidget(m_sections, 1);
}

bool SettingsPage::isShowingUpdates() const
{
    return m_sections->currentIndex() == 3;
}

void SettingsPage::showConnection()
{
    m_sections->setCurrentIndexAnimated(0);
}

void SettingsPage::showFolderSync()
{
    m_sections->setCurrentIndexAnimated(1);
}

void SettingsPage::showAppearance()
{
    m_sections->setCurrentIndexAnimated(2);
}

void SettingsPage::showUpdates()
{
    m_sections->setCurrentIndexAnimated(3);
}

void SettingsPage::showAbout()
{
    m_sections->setCurrentIndexAnimated(4);
}

} // namespace Aurora
