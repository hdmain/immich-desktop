#pragma once

#include "core/ImmichClient.h"

#include <QDialog>

#include <functional>

class QPushButton;
class QStackedWidget;

namespace Aurora {

class ThemeManager;
class VideoPlayerWidget;
class ZoomPanWidget;

class MediaViewerDialog final : public QDialog {
    Q_OBJECT

public:
    // Returns the asset `step` positions away from `assetId`, or an empty asset.
    using NeighborProvider = std::function<ImmichAsset(const QString &assetId, int step)>;

    MediaViewerDialog(ThemeManager *themeManager, ImmichClient *client,
                      const ImmichAsset &asset, NeighborProvider neighbors,
                      QWidget *parent = nullptr);

    void updateNavigation();

signals:
    void copyRequested(const Aurora::ImmichAsset &asset);
    void downloadRequested(const Aurora::ImmichAsset &asset);
    void trashRequested(const Aurora::ImmichAsset &asset);
    void deleteRequested(const Aurora::ImmichAsset &asset);

private:
    void setAsset(const ImmichAsset &asset, bool autoplay = false);
    void step(int direction);
    void handleAssetsDeleted(const QStringList &assetIds);

    ImmichClient *m_client;
    NeighborProvider m_neighbors;
    ImmichAsset m_asset;
    ImmichAsset m_previousAsset;
    ImmichAsset m_nextAsset;
    int m_lastDirection = 1;
    QStackedWidget *m_stack;
    ZoomPanWidget *m_preview;
    VideoPlayerWidget *m_videoPlayer;
    QPushButton *m_previousButton;
    QPushButton *m_nextButton;
    QPushButton *m_copyButton;
};

} // namespace Aurora
