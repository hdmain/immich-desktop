#include "ui/widgets/SelectionBar.h"

#include "ui/IconUtils.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>

namespace Aurora {

SelectionBar::SelectionBar(QWidget *parent)
    : QFrame(parent)
    , m_count(new QLabel(this))
    , m_selectAll(new QPushButton(this))
    , m_open(new QPushButton(this))
    , m_download(new QPushButton(this))
    , m_favorite(new QPushButton(this))
    , m_archive(new QPushButton(this))
    , m_trash(new QPushButton(this))
    , m_delete(new QPushButton(this))
    , m_restore(new QPushButton(this))
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
    configureActionButton(m_selectAll, QStringLiteral(":/icons/circle-check.svg"));
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
    for (QPushButton *button : {m_selectAll, m_open, m_download, m_favorite, m_archive, m_restore,
                                m_trash, m_delete}) {
        button->setCursor(Qt::PointingHandCursor);
        layout->addWidget(button);
    }
    clear->setCursor(Qt::PointingHandCursor);

    connect(clear, &QPushButton::clicked, this, &SelectionBar::clearRequested);
    connect(m_selectAll, &QPushButton::clicked, this, &SelectionBar::selectAllRequested);
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
    fitDensity();
}

void SelectionBar::setAllFavorite(bool allFavorite)
{
    m_allFavorite = allFavorite;
    updateButtons();
}

void SelectionBar::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    fitDensity();
}

void SelectionBar::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);
    fitDensity();
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
    fitDensity();
}

void SelectionBar::fitDensity()
{
    if (m_fitting || width() <= 0)
        return;

    m_fitting = true;
    // Prefer the fullest labels that still fit this bar's width.
    for (const Density density : {Density::Full, Density::Short, Density::Icons}) {
        applyDensity(density);
        if (auto *barLayout = layout()) {
            barLayout->activate();
            if (barLayout->totalSizeHint().width() <= width() || density == Density::Icons) {
                m_density = density;
                break;
            }
        } else {
            m_density = density;
            break;
        }
    }
    m_fitting = false;
}

void SelectionBar::applyDensity(Density density)
{
    const bool icons = density == Density::Icons;
    const bool shortLabels = density == Density::Short;

    const QString selectAll = tr("Select all");
    const QString open = tr("Open");
    const QString download = tr("Download");
    const QString favoriteFull =
        m_allFavorite ? tr("Remove from favorites") : tr("Add to favorites");
    const QString favoriteShort = m_allFavorite ? tr("Unfavorite") : tr("Favorite");
    // "Archive" alone can read like a zip; Immich's Archive is the destination.
    const QString archiveFull =
        m_mode == Mode::Archive ? tr("Move out of Archive") : tr("Move to Archive");
    const QString archiveShort =
        m_mode == Mode::Archive ? tr("Unarchive") : tr("Archive");
    const QString trashFull = tr("Move to trash");
    const QString trashShort = tr("Trash");
    const QString deleteFull = tr("Delete permanently");
    const QString deleteShort = tr("Delete");
    const QString restore = tr("Restore");

    setButtonText(m_selectAll, icons ? QString() : selectAll, selectAll);
    m_selectAll->setToolTip(tr("Select all (Ctrl+A)"));
    setButtonText(m_open, icons ? QString() : open, open);
    setButtonText(m_download, icons ? QString() : download, download);
    setButtonText(m_favorite, icons ? QString() : (shortLabels ? favoriteShort : favoriteFull),
                  favoriteFull);
    setButtonText(m_archive, icons ? QString() : (shortLabels ? archiveShort : archiveFull),
                  archiveFull);
    setButtonText(m_trash, icons ? QString() : (shortLabels ? trashShort : trashFull), trashFull);
    setButtonText(m_delete, icons ? QString() : (shortLabels ? deleteShort : deleteFull),
                  deleteFull);
    setButtonText(m_restore, icons ? QString() : restore, restore);
}

void SelectionBar::setButtonText(QPushButton *button, const QString &visible,
                                 const QString &fullName)
{
    setActionButtonLabel(button, visible);
    button->setToolTip(fullName);
    button->setAccessibleName(fullName);
}

bool confirmBulkChange(QWidget *parent, int count, const QString &title, const QString &question)
{
    constexpr int kAskAbove = 50;
    return count <= kAskAbove ||
           QMessageBox::question(parent, title, question) == QMessageBox::Yes;
}

} // namespace Aurora
