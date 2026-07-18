#include "ui/MainWindow.h"

#include "core/ThemeManager.h"
#include "core/UpdateManager.h"
#include "ui/pages/AppearancePage.h"
#include "ui/pages/DashboardPage.h"
#include "ui/pages/UpdatesPage.h"
#include "ui/widgets/AnimatedStackedWidget.h"
#include "ui/widgets/Sidebar.h"
#include "ui/widgets/TopBar.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainterPath>
#include <QRegion>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

#ifdef Q_OS_WIN
#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>
#endif

namespace Aurora {

class ResizeHandle final : public QWidget {
public:
    ResizeHandle(Qt::Edges edges, QWidget *parent)
        : QWidget(parent)
        , m_edges(edges)
    {
        if (edges == Qt::TopEdge || edges == Qt::BottomEdge)
            setCursor(Qt::SizeVerCursor);
        else if (edges == Qt::LeftEdge || edges == Qt::RightEdge)
            setCursor(Qt::SizeHorCursor);
        else if (edges == (Qt::TopEdge | Qt::LeftEdge) ||
                 edges == (Qt::BottomEdge | Qt::RightEdge))
            setCursor(Qt::SizeFDiagCursor);
        else
            setCursor(Qt::SizeBDiagCursor);
    }

    Qt::Edges edges() const { return m_edges; }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            if (auto *handle = window()->windowHandle())
                handle->startSystemResize(m_edges);
        }
        QWidget::mousePressEvent(event);
    }

private:
    Qt::Edges m_edges;
};

MainWindow::MainWindow(ThemeManager *themeManager, UpdateManager *updateManager, QWidget *parent)
    : QMainWindow(parent)
    , m_topBar(new TopBar(themeManager, this))
    , m_pages(new AnimatedStackedWidget(this))
    , m_updateManager(updateManager)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint |
                   Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint |
                   Qt::WindowSystemMenuHint);
    setWindowTitle(QStringLiteral("immich"));
    setWindowIcon(QIcon(QStringLiteral(":/branding/immich-logo.png")));
    setMinimumSize(900, 620);
    resize(1180, 760);

    auto *surface = new QWidget(this);
    surface->setObjectName(QStringLiteral("windowSurface"));
    surface->setAttribute(Qt::WA_StyledBackground, true);
    setCentralWidget(surface);

    auto *root = new QVBoxLayout(surface);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(10);
    root->addWidget(m_topBar);

    auto *body = new QWidget(surface);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(14);

    auto *sidebar = new Sidebar(themeManager, body);
    bodyLayout->addWidget(sidebar);

    auto *workspace = new QWidget(body);
    auto *workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);

    m_pages->addWidget(new DashboardPage(m_pages));
    m_pages->addWidget(new AppearancePage(themeManager, m_pages));
    m_pages->addWidget(new UpdatesPage(updateManager, m_pages));
    workspaceLayout->addWidget(m_pages, 1);
    bodyLayout->addWidget(workspace, 1);
    root->addWidget(body, 1);

    connect(sidebar, &Sidebar::pageRequested, this, &MainWindow::selectPage);
    connect(m_topBar, &TopBar::updatesRequested, this, [this] { selectPage(2); });
    connect(m_updateManager, &UpdateManager::updateAvailable, this, [this](const UpdateInfo &info) {
        m_topBar->setUpdateAvailable(true, info.version);
        notifyUpdateAvailable();
    });
    connect(m_updateManager, &UpdateManager::upToDate, this, [this] {
        m_topBar->setUpdateAvailable(false);
    });
    connect(m_updateManager, &UpdateManager::stateChanged, this, [this](UpdateState state) {
        if (state != UpdateState::Available && state != UpdateState::ReadyToInstall &&
            state != UpdateState::Downloading)
            m_topBar->setUpdateAvailable(false);
    });

#ifndef Q_OS_WIN
    const QList<Qt::Edges> resizeEdges = {
        Qt::TopEdge, Qt::BottomEdge, Qt::LeftEdge, Qt::RightEdge,
        Qt::TopEdge | Qt::LeftEdge, Qt::TopEdge | Qt::RightEdge,
        Qt::BottomEdge | Qt::LeftEdge, Qt::BottomEdge | Qt::RightEdge
    };
    for (Qt::Edges edges : resizeEdges)
        m_resizeHandles.append(new ResizeHandle(edges, this));
#endif
}

void MainWindow::selectPage(int index)
{
    static const QStringList titles = {
        QStringLiteral("Overview"),
        QStringLiteral("Appearance"),
        QStringLiteral("Updates")
    };
    if (index < 0 || index >= m_pages->count())
        return;
    m_topBar->setPageTitle(titles.value(index));
    m_pages->setCurrentIndexAnimated(index);
}

void MainWindow::scheduleAutoUpdateCheck()
{
    if (m_autoCheckScheduled || !m_updateManager->shouldAutoCheck())
        return;
    m_autoCheckScheduled = true;
    QTimer::singleShot(2500, this, [this] {
        m_updateManager->checkForUpdates(true);
    });
}

void MainWindow::notifyUpdateAvailable()
{
    if (m_pages->currentIndex() == 2)
        return;

    const auto info = m_updateManager->availableUpdate();
    QMessageBox box(this);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("Update available"));
    box.setText(tr("immich desktop v%1 is available.").arg(info.version));
    box.setInformativeText(
        tr("Open the Updates page to download and install the package for this platform."));
    box.addButton(tr("Open Updates"), QMessageBox::AcceptRole);
    box.addButton(tr("Later"), QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() &&
        box.buttonRole(box.clickedButton()) == QMessageBox::AcceptRole) {
        selectPage(2);
    }
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    Q_UNUSED(eventType);
    auto *nativeMessage = static_cast<MSG *>(message);
    if (nativeMessage->message == WM_NCHITTEST && !isMaximized()) {
        const QPoint cursor(GET_X_LPARAM(nativeMessage->lParam),
                            GET_Y_LPARAM(nativeMessage->lParam));
        const QRect bounds = frameGeometry();
        constexpr int resizeBorder = 8;
        const bool left = cursor.x() >= bounds.left() &&
                          cursor.x() < bounds.left() + resizeBorder;
        const bool right = cursor.x() <= bounds.right() &&
                           cursor.x() > bounds.right() - resizeBorder;
        const bool top = cursor.y() >= bounds.top() &&
                         cursor.y() < bounds.top() + resizeBorder;
        const bool bottom = cursor.y() <= bounds.bottom() &&
                            cursor.y() > bounds.bottom() - resizeBorder;

        if (top && left) *result = HTTOPLEFT;
        else if (top && right) *result = HTTOPRIGHT;
        else if (bottom && left) *result = HTBOTTOMLEFT;
        else if (bottom && right) *result = HTBOTTOMRIGHT;
        else if (left) *result = HTLEFT;
        else if (right) *result = HTRIGHT;
        else if (top) *result = HTTOP;
        else if (bottom) *result = HTBOTTOM;
        else return QMainWindow::nativeEvent(eventType, message, result);
        return true;
    }
#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        applyWindowCorners();
        updateResizeHandles();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (isVisible()) {
        applyWindowCorners();
        updateResizeHandles();
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    applyWindowCorners();
    updateResizeHandles();
    scheduleAutoUpdateCheck();
}

void MainWindow::applyWindowCorners()
{
    const bool squareCorners = isMaximized() || isFullScreen();

#ifdef Q_OS_WIN
    const HWND handle = reinterpret_cast<HWND>(winId());
    const int cornerPreference = squareCorners ? 1 : 2; // DoNotRound / Round
    constexpr DWORD cornerPreferenceAttribute = 33; // DWMWA_WINDOW_CORNER_PREFERENCE
    const HRESULT result = DwmSetWindowAttribute(
        handle, cornerPreferenceAttribute, &cornerPreference, sizeof(cornerPreference));
    if (SUCCEEDED(result)) {
        clearMask();
        return;
    }
#endif

    if (squareCorners) {
        clearMask();
        return;
    }

    constexpr qreal radius = 14.0;
    QPainterPath path;
    path.addRoundedRect(QRectF(rect()), radius, radius);
    setMask(QRegion(path.toFillPolygon().toPolygon()));
}

void MainWindow::updateResizeHandles()
{
    if (m_resizeHandles.isEmpty())
        return;

    const bool enabled = !isMaximized() && !isFullScreen();
    constexpr int edgeSize = 8;
    constexpr int cornerSize = 14;

    for (auto *handle : m_resizeHandles) {
        handle->setVisible(enabled);
        if (!enabled)
            continue;

        const Qt::Edges edges = handle->edges();
        if (edges == Qt::TopEdge)
            handle->setGeometry(cornerSize, 0, width() - 2 * cornerSize, edgeSize);
        else if (edges == Qt::BottomEdge)
            handle->setGeometry(cornerSize, height() - edgeSize,
                                width() - 2 * cornerSize, edgeSize);
        else if (edges == Qt::LeftEdge)
            handle->setGeometry(0, cornerSize, edgeSize, height() - 2 * cornerSize);
        else if (edges == Qt::RightEdge)
            handle->setGeometry(width() - edgeSize, cornerSize,
                                edgeSize, height() - 2 * cornerSize);
        else if (edges == (Qt::TopEdge | Qt::LeftEdge))
            handle->setGeometry(0, 0, cornerSize, cornerSize);
        else if (edges == (Qt::TopEdge | Qt::RightEdge))
            handle->setGeometry(width() - cornerSize, 0, cornerSize, cornerSize);
        else if (edges == (Qt::BottomEdge | Qt::LeftEdge))
            handle->setGeometry(0, height() - cornerSize, cornerSize, cornerSize);
        else
            handle->setGeometry(width() - cornerSize, height() - cornerSize,
                                cornerSize, cornerSize);
        handle->raise();
    }
}

} // namespace Aurora
