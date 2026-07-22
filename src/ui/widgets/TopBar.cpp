#include "ui/widgets/TopBar.h"

#include "core/ThemeManager.h"
#include "ui/IconUtils.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QWindow>
#include <QtMath>

namespace Aurora {

TopBar::TopBar(ThemeManager *themeManager, QWidget *parent)
    : QWidget(parent)
    , m_themeManager(themeManager)
    , m_title(new QLabel(QStringLiteral("Overview"), this))
    , m_updateButton(new QPushButton(this))
    , m_minimizeButton(new QPushButton(this))
    , m_maximizeButton(new QPushButton(this))
    , m_closeButton(new QPushButton(this))
{
    setObjectName(QStringLiteral("titleBar"));
    setFixedHeight(52);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 6, 0);
    layout->setSpacing(8);

    auto *logo = new QLabel(this);
    const qreal dpr = devicePixelRatioF();
    constexpr int kLogoHeight = 37; // ~20% smaller than max (46) in the 52px title bar
    QPixmap logoPixmap(QStringLiteral(":/branding/immich-logo-inline-light.png"));
    const int logoWidth = logoPixmap.isNull()
        ? qRound(kLogoHeight * (792.0 / 266.25))
        : qMax(1, qRound(kLogoHeight * (qreal(logoPixmap.width()) / qreal(logoPixmap.height()))));
    QPixmap scaled = logoPixmap.scaled(
        QSize(qRound(logoWidth * dpr), qRound(kLogoHeight * dpr)),
        Qt::KeepAspectRatio, Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(dpr);
    logo->setPixmap(scaled);
    logo->setFixedSize(logoWidth, kLogoHeight);
    logo->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    layout->addWidget(logo);

    auto *separator = new QLabel(QStringLiteral("  /  "), this);
    separator->setProperty("subheading", true);
    layout->addWidget(separator);

    m_title->setProperty("section", true);
    layout->addWidget(m_title);
    layout->addStretch();

    m_updateButton->setObjectName(QStringLiteral("updateButton"));
    m_updateButton->setProperty("windowControl", true);
    m_updateButton->setCursor(Qt::PointingHandCursor);
    m_updateButton->setFixedHeight(36);
    m_updateButton->setIconSize(QSize(18, 18));
    m_updateButton->setVisible(false);
    layout->addWidget(m_updateButton);

    auto prepareControl = [layout](QPushButton *button, const QString &name) {
        button->setObjectName(name);
        button->setProperty("windowControl", true);
        button->setCursor(Qt::PointingHandCursor);
        button->setFixedSize(42, 36);
        button->setIconSize(QSize(18, 18));
        layout->addWidget(button);
    };

    prepareControl(m_minimizeButton, QStringLiteral("minimizeButton"));
    prepareControl(m_maximizeButton, QStringLiteral("maximizeButton"));
    prepareControl(m_closeButton, QStringLiteral("closeButton"));

    connect(m_updateButton, &QPushButton::clicked, this, &TopBar::updatesRequested);
    connect(m_minimizeButton, &QPushButton::clicked, this, [this] { window()->showMinimized(); });
    connect(m_maximizeButton, &QPushButton::clicked, this, &TopBar::toggleMaximized);
    connect(m_closeButton, &QPushButton::clicked, this, [this] { window()->close(); });
    connect(m_themeManager, &ThemeManager::appearanceChanged, this, &TopBar::refreshIcons);
    refreshIcons();
}

void TopBar::setPageTitle(const QString &title)
{
    m_title->setText(title);
}

void TopBar::setUpdateAvailable(bool available, const QString &version)
{
    m_updateButton->setVisible(available);
    if (available) {
        m_updateButton->setText(version.isEmpty()
                                    ? tr("  Update available")
                                    : tr("  Update to v%1").arg(version));
        m_updateButton->setToolTip(tr("Open the Updates page"));
    }
    refreshIcons();
}

void TopBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        toggleMaximized();
    QWidget::mouseDoubleClickEvent(event);
}

void TopBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !window()->isMaximized()) {
        if (auto *handle = window()->windowHandle())
            handle->startSystemMove();
    }
    QWidget::mousePressEvent(event);
}

void TopBar::toggleMaximized()
{
    if (window()->isMaximized()) {
        window()->showNormal();
    } else {
        window()->showMaximized();
    }
    refreshIcons();
}

void TopBar::refreshIcons()
{
    const QColor color = m_themeManager->palette().text;
    const qreal scale = devicePixelRatioF();
    m_updateButton->setIcon(QIcon(renderSvgIcon(
        QStringLiteral(":/icons/download.svg"), m_themeManager->palette().accent,
        QSize(18, 18), scale)));
    m_minimizeButton->setIcon(QIcon(renderSvgIcon(
        QStringLiteral(":/icons/minus.svg"), color, QSize(18, 18), scale)));
    m_maximizeButton->setIcon(QIcon(renderSvgIcon(
        window()->isMaximized() ? QStringLiteral(":/icons/copy.svg")
                                : QStringLiteral(":/icons/square.svg"),
        color, QSize(16, 16), scale)));
    m_closeButton->setIcon(QIcon(renderSvgIcon(
        QStringLiteral(":/icons/x.svg"), color, QSize(18, 18), scale)));
}

} // namespace Aurora
