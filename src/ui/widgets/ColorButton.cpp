#include "ui/widgets/ColorButton.h"

#include <QColorDialog>
#include <QPainter>

namespace Aurora {

ColorButton::ColorButton(const QString &label, ColorRole role, ThemeManager *themeManager,
                         QWidget *parent)
    : QPushButton(parent)
    , m_label(label)
    , m_role(role)
    , m_themeManager(themeManager)
{
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(48);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(this, &QPushButton::clicked, this, &ColorButton::chooseColor);
    connect(m_themeManager, &ThemeManager::appearanceChanged, this, [this] { update(); });
}

void ColorButton::paintEvent(QPaintEvent *)
{
    const auto &p = m_themeManager->palette();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setPen(QPen(p.border, 1));
    painter.setBrush(p.button);
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 11, 11);

    painter.setPen(p.text);
    painter.drawText(rect().adjusted(14, 0, -54, 0), Qt::AlignVCenter | Qt::AlignLeft, m_label);

    const QRect swatch(width() - 42, (height() - 24) / 2, 24, 24);
    painter.setPen(QPen(p.border, 1));
    painter.setBrush(roleColor());
    painter.drawRoundedRect(swatch, 7, 7);
}

QColor ColorButton::roleColor() const
{
    const auto &p = m_themeManager->customPalette();
    switch (m_role) {
    case ColorRole::Background: return p.background;
    case ColorRole::Panel: return p.panel;
    case ColorRole::Button: return p.button;
    case ColorRole::Accent: return p.accent;
    }
    return {};
}

void ColorButton::chooseColor()
{
    const QColor color = QColorDialog::getColor(
        roleColor(), this, tr("Choose %1 color").arg(m_label),
        QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    if (color.isValid())
        m_themeManager->setCustomColor(m_role, color);
}

} // namespace Aurora
