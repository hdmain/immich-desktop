#include "ui/pages/SettingsPage.h"

#include "core/ThemeManager.h"
#include "ui/pages/AppearancePage.h"
#include "ui/pages/ConnectionPage.h"
#include "ui/pages/UpdatesPage.h"
#include "ui/widgets/AnimatedStackedWidget.h"

#include <QLabel>
#include <QVBoxLayout>

namespace Aurora {

SettingsPage::SettingsPage(ThemeManager *themeManager, UpdateManager *updateManager,
                           ImmichClient *immichClient,
                           QWidget *parent)
    : QWidget(parent)
    , m_sections(new AnimatedStackedWidget(this))
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(4, 6, 4, 4);
    root->setSpacing(8);

    auto *heading = new QLabel(tr("Settings"), this);
    heading->setProperty("heading", true);
    auto *subheading = new QLabel(
        tr("Choose a settings section from the sidebar."), this);
    subheading->setProperty("subheading", true);
    root->addWidget(heading);
    root->addWidget(subheading);

    m_sections->addWidget(new ConnectionPage(immichClient, m_sections));
    m_sections->addWidget(new AppearancePage(themeManager, m_sections));
    m_sections->addWidget(new UpdatesPage(updateManager, m_sections));
    root->addWidget(m_sections, 1);
}

bool SettingsPage::isShowingUpdates() const
{
    return m_sections->currentIndex() == 2;
}

void SettingsPage::showConnection()
{
    m_sections->setCurrentIndexAnimated(0);
}

void SettingsPage::showAppearance()
{
    m_sections->setCurrentIndexAnimated(1);
}

void SettingsPage::showUpdates()
{
    m_sections->setCurrentIndexAnimated(2);
}

} // namespace Aurora
