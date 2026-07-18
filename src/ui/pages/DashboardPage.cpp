#include "ui/pages/DashboardPage.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace Aurora {
namespace {

QFrame *createCard(const QString &eyebrow, const QString &title,
                   const QString &description, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setProperty("card", true);
    card->setMinimumHeight(150);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(8);

    auto *eyebrowLabel = new QLabel(eyebrow, card);
    eyebrowLabel->setProperty("subheading", true);
    auto *titleLabel = new QLabel(title, card);
    titleLabel->setProperty("section", true);
    auto *descriptionLabel = new QLabel(description, card);
    descriptionLabel->setProperty("subheading", true);
    descriptionLabel->setWordWrap(true);

    layout->addWidget(eyebrowLabel);
    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);
    layout->addStretch();
    return card;
}

} // namespace

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 6, 4, 4);
    layout->setSpacing(8);

    auto *heading = new QLabel(QStringLiteral("A clean foundation for what comes next."), this);
    heading->setProperty("heading", true);
    auto *subheading = new QLabel(
        QStringLiteral("Add product modules without coupling them to navigation or appearance."),
        this);
    subheading->setProperty("subheading", true);

    layout->addWidget(heading);
    layout->addWidget(subheading);
    layout->addSpacing(22);

    auto *grid = new QGridLayout;
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(14);
    grid->addWidget(createCard(QStringLiteral("SYSTEM"), QStringLiteral("Runtime themes"),
                               QStringLiteral("The entire interface responds instantly to palette changes."),
                               this), 0, 0);
    grid->addWidget(createCard(QStringLiteral("ARCHITECTURE"), QStringLiteral("Modular by design"),
                               QStringLiteral("Pages, controls, persistence, and appearance remain independently extensible."),
                               this), 0, 1);
    grid->addWidget(createCard(QStringLiteral("WORKSPACE"), QStringLiteral("Future module"),
                               QStringLiteral("This card can become analytics, media, account, or workflow functionality."),
                               this), 1, 0);
    grid->addWidget(createCard(QStringLiteral("WORKSPACE"), QStringLiteral("Future module"),
                               QStringLiteral("Use the central theme manager to keep every new surface visually consistent."),
                               this), 1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    layout->addLayout(grid);
    layout->addStretch();
}

} // namespace Aurora
