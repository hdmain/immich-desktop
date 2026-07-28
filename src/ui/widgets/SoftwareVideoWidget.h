#pragma once

#include <QImage>
#include <QWidget>

namespace Aurora {

// CPU-side video surface for QVideoSink. Avoids QVideoWidget's OpenGL/EGL path,
// which fails or hangs on some Wayland/XWayland setups.
class SoftwareVideoWidget final : public QWidget {
    Q_OBJECT

public:
    enum class ScaleMode {
        Fit,   // letterbox
        Cover, // fill widget, crop overflow (hover tiles)
    };

    explicit SoftwareVideoWidget(QWidget *parent = nullptr);

    void setFrame(QImage frame);
    void clearFrame();
    void setScaleMode(ScaleMode mode);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_frame;
    ScaleMode m_scaleMode = ScaleMode::Fit;
};

} // namespace Aurora
