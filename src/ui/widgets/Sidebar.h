#pragma once

#include "ui/BrowseSection.h"

#include <QHash>
#include <QWidget>

class QButtonGroup;
class QLabel;
class QScrollArea;
class QVariantAnimation;
class QVBoxLayout;

namespace Aurora {

class AnimatedButton;
class BrowseTree;
class ThemeManager;

class Sidebar final : public QWidget {
    Q_OBJECT

public:
    explicit Sidebar(ThemeManager *themeManager, QWidget *parent = nullptr);
    void setCurrentPage(int index);
    void setActiveSection(BrowseSection section, const QString &itemValue = {});
    void toggleCollapsed();
    void setNarrow(bool narrow);
    BrowseTree *browseTree(BrowseSection section) const;
    bool isSectionExpanded(BrowseSection section) const;

signals:
    void pageRequested(int index);
    void sectionRequested(Aurora::BrowseSection section);
    void sectionExpanded(Aurora::BrowseSection section);

private:
    AnimatedButton *addNavigation(const QString &label, const QString &icon, int index,
                                  bool checkable = true);
    void addSectionNavigation(BrowseSection section, const QString &label,
                              const QString &icon);
    void clearNavigation();
    void showMainNavigation();
    void showSettingsNavigation();
    bool isCollapsed() const;
    void refreshLayout(bool animate);
    void updateTreeVisibility();
    void expandSection(BrowseSection section);

    ThemeManager *m_themeManager;
    QLabel *m_navigationLabel;
    QVBoxLayout *m_navigationLayout;
    QButtonGroup *m_navigationGroup;
    bool m_settingsMode = false;
    QLabel *m_version;
    AnimatedButton *m_toggleButton;
    QVariantAnimation *m_widthAnimation;
    bool m_collapsed = false;
    bool m_narrow = false;
    bool m_expandedWhileNarrow = false;
    QHash<BrowseSection, BrowseTree *> m_trees;
    QHash<BrowseSection, bool> m_treeExpanded;
    QScrollArea *m_scroll = nullptr;
};

} // namespace Aurora
