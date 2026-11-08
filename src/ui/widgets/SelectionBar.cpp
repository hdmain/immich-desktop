#include "ui/widgets/SelectionBar.h"

#include "ui/IconUtils.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>

namespace Aurora {

SelectionBar::SelectionBar(QWidget *parent)
    : QFrame(parent)
    , m_count(new QLabel(this))
    , m_open(new QPushButton(tr("Open"), this))
    , m_download(new QPushButton(tr("Download"), this))
    , m_favorite(new QPushButton(this))
    , m_archive(new QPushButton(this))
    , m_trash(new QPushButton(tr("Move to trash"), this))
    , m_delete(new QPushButton(tr("Delete permanently"), this))
    , m_restore(new QPushButton(tr("Restore"), this))
{
    setObjectName(QStringLiteral("selectionBar"));
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *clear = new QPushButton(this);
    configureActionButton(clear, QStringLiteral(":/icons/x.svg"));
    setActionButtonLabel(clear, QString());
    clear->setToolTip(tr("Clear selection (Esc)"));
    clear->setAccessibleName(tr("Clear selection"));
    auto *selectAll = new QPushButton(tr("Select all"), this);
    configureActionButton(selectAll, QStringLiteral(":/icons/circle-check.svg"));
    selectAll->setToolTip(tr("Select all (Ctrl+A)"));
    m_count->setProperty("section", true);

    configureActionButton(m_open, QStringLiteral(":/icons/image.svg"));
    configureActionButton(m_download, QStringLiteral(":/icons/download.svg"));
    configureActionButton(m_favorite, QStringLiteral(":/icons/heart.svg"));
    configureActionButton(m_archive, QStringLiteral(":/icons/archive.svg"));
    configureActionButton(m_trash, QStringLiteral(":/icons/trash.svg"));
    configureActionButton(m_delete, QStringLiteral(":/icons/circle-x.svg"));
    configureActionButton(m_restore, QStringLiteral(":/icons/rotate-ccw.svg"));

    layout->addWidget(clear);
    layout->addWidget(m_count);
    layout->addStretch(1);
    for (QPushButton *button : {selectAll, m_open, m_download, m_favorite, m_archive, m_restore,
                                m_trash, m_delete}) {
        button->setCursor(Qt::PointingHandCursor);
        layout->addWidget(button);
    }
    clear->setCursor(Qt::PointingHandCursor);

    connect(clear, &QPushButton::clicked, this, &SelectionBar::clearRequested);
    connect(selectAll, &QPushButton::clicked, this, &SelectionBar::selectAllRequested);
    connect(m_open, &QPushButton::clicked, this, &SelectionBar::openRequested);
    connect(m_download, &QPushButton::clicked, this, &SelectionBar::downloadRequested);
    connect(m_favorite, &QPushButton::clicked, this,
            [this] { emit favoriteRequested(!m_allFavorite); });
    connect(m_archive, &QPushButton::clicked, this,
            [this] { emit archiveRequested(m_mode != Mode::Archive); });
    connect(m_trash, &QPushButton::clicked, this, &SelectionBar::trashRequested);
    connect(m_delete, &QPushButton::clicked, this, &SelectionBar::deleteRequested);
    connect(m_restore, &QPushButton::clicked, this, &SelectionBar::restoreRequested);
    updateButtons();
}

void SelectionBar::setMode(Mode mode)
{
    m_mode = mode;
    updateButtons();
}

void SelectionBar::setCount(int count)
{
    m_count->setText(tr("%n selected", nullptr, count));
}

void SelectionBar::setAllFavorite(bool allFavorite)
{
    m_allFavorite = allFavorite;
    updateButtons();
}

void SelectionBar::updateButtons()
{
    const bool trash = m_mode == Mode::Trash;
    m_favorite->setVisible(!trash);
    m_archive->setVisible(!trash);
    m_trash->setVisible(!trash);
    m_restore->setVisible(trash);
    m_favorite->setProperty("actionIcon", m_allFavorite ? QStringLiteral(":/icons/heart-filled.svg")
                                                        : QStringLiteral(":/icons/heart.svg"));
    setActionButtonLabel(m_favorite, m_allFavorite ? tr("Remove from favorites")
                                                   : tr("Add to favorites"));
    // "Archive" alone reads like a zip file; this is Immich's Archive.
    setActionButtonLabel(m_archive, m_mode == Mode::Archive ? tr("Move out of Archive")
                                                           : tr("Move to Archive"));
}

bool confirmBulkChange(QWidget *parent, int count, const QString &title, const QString &question)
{
    constexpr int kAskAbove = 50;
    return count <= kAskAbove ||
           QMessageBox::question(parent, title, question) == QMessageBox::Yes;
}

} // namespace Aurora
