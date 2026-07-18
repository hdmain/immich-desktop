#pragma once

#include "core/ThemeManager.h"

#include <QPushButton>

namespace Aurora {

class ColorButton final : public QPushButton {
    Q_OBJECT

public:
    ColorButton(const QString &label, ColorRole role, ThemeManager *themeManager,
                QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor roleColor() const;
    void chooseColor();

    QString m_label;
    ColorRole m_role;
    ThemeManager *m_themeManager;
};

} // namespace Aurora
