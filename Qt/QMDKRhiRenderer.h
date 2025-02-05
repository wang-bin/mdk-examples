/*
 * Copyright (c) 2025 WangBin <wbsecg1 at gmail.com>
 */
#ifndef QMDKRHIWIDGETRENDERER_H
#define QMDKRHIWIDGETRENDERER_H

#include <QRhiWidget>
#include <rhi/qrhi.h>
#include <memory>

namespace mdk {
class Player;
}
class QMDKPlayer;

#ifndef Q_MDK_API
#define Q_MDK_API Q_DECL_IMPORT
#endif

class Q_MDK_API QMDKRhiWidgetRenderer : public QRhiWidget
{
    Q_OBJECT
public:
    using ThisClass = QMDKRhiWidgetRenderer;

    QMDKRhiWidgetRenderer(QWidget *parent = nullptr);
    ~QMDKRhiWidgetRenderer() override;

    void setVsync(bool value) {
        m_vsync = value;
        setProperty("vsync", value);
    }

    void setSource(QMDKPlayer* player);
    QMDKPlayer* source() const { return m_player; }
    void setROI(float x, float y, float w, float h);

Q_SIGNALS:
    void videoSizeChanged(const QSize& size);

protected:
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;
private:
    QMDKPlayer* m_player = nullptr;
    QRhiCommandBuffer *m_cb = nullptr;
    bool m_vsync = false;
};

#endif // QMDKRHIWIDGETRENDERER_H
