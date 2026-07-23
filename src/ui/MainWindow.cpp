#include "ui/MainWindow.h"

#include "AppVersion.h"
#include "core/AppSettings.h"
#include "core/ImmichClient.h"
#include "core/ThemeManager.h"
#include "core/UpdateManager.h"
#include "ui/AppIcon.h"
#include "ui/pages/ExplorePage.h"
#include "ui/pages/LibraryPage.h"
#include "ui/pages/SettingsPage.h"
#include "ui/widgets/AnimatedStackedWidget.h"
#include "ui/widgets/Sidebar.h"
#include "ui/widgets/TopBar.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainterPath>
#include <QRegion>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>
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
        setAttribute(Qt::WA_TranslucentBackground);
        setAutoFillBackground(false);
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

MainWindow::MainWindow(ThemeManager *themeManager, UpdateManager *updateManager,
                       ImmichClient *immichClient, QWidget *parent)
    : QMainWindow(parent)
    , m_topBar(new TopBar(themeManager, this))
    , m_pages(new AnimatedStackedWidget(this))
    , m_settingsPage(new SettingsPage(themeManager, updateManager, immichClient, this))
    , m_sidebar(new Sidebar(themeManager, this))
    , m_themeManager(themeManager)
    , m_updateManager(updateManager)
    , m_immichClient(immichClient)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint |
                   Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint |
                   Qt::WindowSystemMenuHint);
    setWindowTitle(QStringLiteral("immich"));
    setWindowIcon(applicationIconForPalette(m_themeManager->palette()));
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

    bodyLayout->addWidget(m_sidebar);

    auto *workspace = new QWidget(body);
    auto *workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);

    m_pages->addWidget(new LibraryPage(immichClient, m_pages));
    m_pages->addWidget(new ExplorePage(immichClient, m_pages));
    m_pages->addWidget(m_settingsPage);
    workspaceLayout->addWidget(m_pages, 1);
    bodyLayout->addWidget(workspace, 1);
    root->addWidget(body, 1);

    connect(m_sidebar, &Sidebar::pageRequested, this, &MainWindow::selectPage);
    connect(m_topBar, &TopBar::updatesRequested, this, [this] {
        selectPage(4);
    });
    connect(m_updateManager, &UpdateManager::updateAvailable, this, [this](const UpdateInfo &info) {
        m_topBar->setUpdateAvailable(true, info.version);
        if (m_notifiedUpdateVersion != info.version) {
            m_notifiedUpdateVersion = info.version;
            notifyUpdateAvailable();
        }
    });
    connect(m_updateManager, &UpdateManager::upToDate, this, [this] {
        m_topBar->setUpdateAvailable(false);
        m_notifiedUpdateVersion.clear();
    });
    connect(m_updateManager, &UpdateManager::stateChanged, this, [this](UpdateState state) {
        if (state != UpdateState::Available && state != UpdateState::ReadyToInstall &&
            state != UpdateState::Downloading)
            m_topBar->setUpdateAvailable(false);
        refreshTrayIcon();
    });
    connect(m_themeManager, &ThemeManager::appearanceChanged, this, [this] {
        setWindowIcon(applicationIconForPalette(m_themeManager->palette()));
        refreshTrayIcon();
    });
    connect(m_immichClient, &ImmichClient::uploadQueueChanged, this,
            [this](int) { refreshTrayIcon(); });
    connect(m_immichClient, &ImmichClient::transferActivityChanged, this,
            &MainWindow::refreshTrayIcon);

    const QList<Qt::Edges> resizeEdges = {
        Qt::TopEdge, Qt::BottomEdge, Qt::LeftEdge, Qt::RightEdge,
        Qt::TopEdge | Qt::LeftEdge, Qt::TopEdge | Qt::RightEdge,
        Qt::BottomEdge | Qt::LeftEdge, Qt::BottomEdge | Qt::RightEdge
    };
    for (Qt::Edges edges : resizeEdges) {
        auto *handle = new ResizeHandle(edges, this);
        handle->setAttribute(Qt::WA_StyledBackground, false);
        m_resizeHandles.append(handle);
    }

    setupTrayIcon();
}

void MainWindow::setupTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    QApplication::setQuitOnLastWindowClosed(false);

    m_trayIcon = new QSystemTrayIcon(this);
    refreshTrayIcon();
    m_trayIcon->setToolTip(tr("immich"));

    auto *menu = new QMenu(this);
    menu->addAction(tr("Show immich"), this, &MainWindow::raiseToFront);
    menu->addSeparator();
    menu->addAction(tr("Quit"), this, &MainWindow::quitApplication);
    m_trayIcon->setContextMenu(menu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger ||
                    reason == QSystemTrayIcon::DoubleClick)
                    raiseToFront();
            });

    m_trayIcon->show();
}

void MainWindow::raiseToFront()
{
    if (isMinimized() || !isVisible())
        setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    show();
    showNormal();
    raise();
    activateWindow();

#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd) {
        if (IsIconic(hwnd))
            ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        BringWindowToTop(hwnd);
    }
#endif
}

void MainWindow::quitApplication()
{
    m_forceQuit = true;
    QApplication::quit();
}

bool MainWindow::closeToTrayEnabled() const
{
    return AppSettings().loadWindow().closeToTray;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_forceQuit && closeToTrayEnabled() && m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::selectPage(int index)
{
    if (index < 0 || index > 4)
        return;

    if (index == 0) {
        m_topBar->setPageTitle(QStringLiteral("Library"));
        m_pages->setCurrentIndexAnimated(0);
    } else if (index == 1) {
        m_topBar->setPageTitle(QStringLiteral("Explore"));
        m_pages->setCurrentIndexAnimated(1);
    } else {
        if (index == 2) {
            m_settingsPage->showConnection();
            m_topBar->setPageTitle(QStringLiteral("Settings / Immich Server"));
        } else if (index == 3) {
            m_settingsPage->showAppearance();
            m_topBar->setPageTitle(QStringLiteral("Settings / Appearance"));
        } else {
            m_settingsPage->showUpdates();
            m_topBar->setPageTitle(QStringLiteral("Settings / Update"));
        }
        m_pages->setCurrentIndexAnimated(2);
    }
    m_sidebar->setCurrentPage(index);
}

void MainWindow::scheduleStartupUpdateCheck()
{
    if (m_autoCheckScheduled || !m_updateManager->settings().autoCheck)
        return;
    m_autoCheckScheduled = true;
    QTimer::singleShot(2500, this, [this] {
        m_updateManager->checkForUpdates(true);
    });
}

void MainWindow::scheduleGitHubStarPrompt()
{
    if (m_starPromptScheduled)
        return;
    m_starPromptScheduled = true;

    AppSettings store;
    auto support = store.loadSupport();
    ++support.launchCount;
    store.saveSupport(support);

    // Soft ask after a few launches; never nag once dismissed.
    if (support.githubStarDismissed || support.launchCount < 3)
        return;

    QTimer::singleShot(8000, this, &MainWindow::maybeShowGitHubStarPrompt);
}

void MainWindow::maybeShowGitHubStarPrompt()
{
    AppSettings store;
    auto support = store.loadSupport();
    if (support.githubStarDismissed || !isVisible())
        return;

    QMessageBox box(this);
    box.setAttribute(Qt::WA_StyledBackground, true);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("Support immich desktop"));
    box.setText(tr("If immich desktop is useful, a GitHub star helps a lot."));
    box.setInformativeText(
        tr("Stars make the project easier to discover. Thanks for considering it!"));
    box.addButton(tr("Star on GitHub"), QMessageBox::AcceptRole);
    box.addButton(tr("Maybe later"), QMessageBox::RejectRole);
    box.exec();

    support = store.loadSupport();
    support.githubStarDismissed = true;
    store.saveSupport(support);

    if (box.clickedButton() &&
        box.buttonRole(box.clickedButton()) == QMessageBox::AcceptRole) {
        QDesktopServices::openUrl(
            QUrl(QStringLiteral("https://github.com/%1")
                     .arg(QString::fromLatin1(Config::GitHubRepository))));
    }
}

void MainWindow::refreshTrayIcon()
{
    if (!m_trayIcon)
        return;

    const auto &palette = m_themeManager->palette();
    if (m_immichClient && m_immichClient->isUploading()) {
        m_trayIcon->setIcon(trayUploadIcon(palette));
        m_trayIcon->setToolTip(tr("immich — uploading"));
        return;
    }

    const bool downloadingUpdate = m_updateManager &&
                                   m_updateManager->state() == UpdateState::Downloading;
    if ((m_immichClient && m_immichClient->isDownloading()) || downloadingUpdate) {
        m_trayIcon->setIcon(trayDownloadIcon(palette));
        m_trayIcon->setToolTip(tr("immich — downloading"));
        return;
    }

    m_trayIcon->setIcon(applicationIconForPalette(palette));
    m_trayIcon->setToolTip(tr("immich"));
}

void MainWindow::notifyUpdateAvailable()
{
    if (m_pages->currentIndex() == 2 && m_settingsPage->isShowingUpdates())
        return;

    const auto info = m_updateManager->availableUpdate();
    if (!isVisible() && m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(
            tr("Update available"),
            tr("immich desktop v%1 is available.").arg(info.version),
            QSystemTrayIcon::Information, 8000);
        return;
    }

    QMessageBox box(this);
    box.setAttribute(Qt::WA_StyledBackground, true);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("Update available"));
    box.setText(tr("immich desktop v%1 is available.").arg(info.version));
    if (!info.releaseNotes.isEmpty()) {
        box.setInformativeText(tr("What's new:"));
        box.setDetailedText(info.releaseNotes);
    } else {
        box.setInformativeText(
            tr("Open the Updates page to download and install the package for this platform."));
    }
    box.addButton(tr("Open Updates"), QMessageBox::AcceptRole);
    box.addButton(tr("Later"), QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() &&
        box.buttonRole(box.clickedButton()) == QMessageBox::AcceptRole) {
        selectPage(4);
    }
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
        return QMainWindow::nativeEvent(eventType, message, result);

    auto *nativeMessage = static_cast<MSG *>(message);
    if (nativeMessage->message == WM_NCCALCSIZE && nativeMessage->wParam) {
        // Keep a frameless client area while retaining WS_THICKFRAME for resizing.
        *result = 0;
        return true;
    }
    if (nativeMessage->message == WM_NCHITTEST && !isMaximized() && !isFullScreen()) {
        const QPoint cursor(GET_X_LPARAM(nativeMessage->lParam),
                            GET_Y_LPARAM(nativeMessage->lParam));
        const QPoint local = mapFromGlobal(cursor);
        constexpr int resizeBorder = 8;
        const bool left = local.x() >= 0 && local.x() < resizeBorder;
        const bool right = local.x() >= width() - resizeBorder && local.x() < width();
        const bool top = local.y() >= 0 && local.y() < resizeBorder;
        const bool bottom = local.y() >= height() - resizeBorder && local.y() < height();

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
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
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
    ensureResizableFrame();
    applyWindowCorners();
    updateResizeHandles();
}

void MainWindow::ensureResizableFrame()
{
#ifdef Q_OS_WIN
    const HWND handle = reinterpret_cast<HWND>(winId());
    const LONG_PTR style = GetWindowLongPtr(handle, GWL_STYLE);
    SetWindowLongPtr(handle, GWL_STYLE,
                     style | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION);
    SetWindowPos(handle, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#else
    // Resize grips cover non-Windows platforms.
#endif
}

void MainWindow::applyWindowCorners()
{
    const bool squareCorners = isMaximized() || isFullScreen();

#ifdef Q_OS_WIN
    const HWND handle = reinterpret_cast<HWND>(winId());
    const int cornerPreference = squareCorners ? 1 : 2; // DoNotRound / Round
    constexpr DWORD cornerPreferenceAttribute = 33; // DWMWA_WINDOW_CORNER_PREFERENCE
    DwmSetWindowAttribute(handle, cornerPreferenceAttribute,
                          &cornerPreference, sizeof(cornerPreference));
    // Avoid QWidget::setMask on Windows — it breaks border hit-testing/resizing.
    clearMask();
#else
    if (squareCorners) {
        clearMask();
        return;
    }

    constexpr qreal radius = 14.0;
    QPainterPath path;
    path.addRoundedRect(QRectF(rect()), radius, radius);
    setMask(QRegion(path.toFillPolygon().toPolygon()));
#endif
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
