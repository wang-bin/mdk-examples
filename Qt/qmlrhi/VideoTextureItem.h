/*
 * Copyright (c) 2020-2023 WangBin <wbsecg1 at gmail.com>
 * MDK SDK in QtQuick RHI
 */
#pragma once
#include <QtQuick/QQuickItem>
#include "mdk/global.h"
namespace MDK_NS {
class Player;
}
using namespace MDK_NS;
class VideoTextureNode;

class VideoTextureItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool autoPlay READ autoPlay WRITE setAutoPlay NOTIFY autoPlayChanged)
    // autoHDR: for macOS/iOS, only work with HDRExtendedSrgbLinear (QSG_RHI_HDR=scrgb) swapchain format
    Q_PROPERTY(bool autoHDR READ autoHDR WRITE setAutoHDR NOTIFY autoHDRChanged FINAL)
#ifdef QML_ELEMENT
    QML_ELEMENT // no need to call qmlRegisterType
#endif
public:
    VideoTextureItem();
    ~VideoTextureItem() override;

    QString source() { return m_source; }
    void setSource(const QString & s);

    bool autoPlay() const { return m_autoPlay; }
    void setAutoPlay(bool value);

    bool autoHDR() const { return m_autoHDR;}
    void setAutoHDR(bool value);

    Q_INVOKABLE void play();

signals:
    void sourceChanged();
    void autoPlayChanged();
    void autoHDRChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#else
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#endif

private slots:
    void invalidateSceneGraph();

private:
    void releaseResources() override;

    friend class VideoTextureNode; // TODO: QSGNode
    VideoTextureNode *m_node = nullptr;
    bool m_autoPlay = true;
    bool m_autoHDR = false;
    QString m_source;
    std::shared_ptr<Player> m_player;
};
//! [1]
