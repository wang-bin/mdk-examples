/*
 * Copyright (c) 2020 WangBin <wbsecg1 at gmail.com>
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
    QML_ELEMENT

public:
    VideoTextureItem();
    ~VideoTextureItem() override;

    QString source() { return m_source; }
    void setSource(const QString & s);

    Q_INVOKABLE void play();

signals:
    void sourceChanged();

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

    friend class VideoTextureNode;
    VideoTextureNode *m_node = nullptr;
    QString m_source;
    std::shared_ptr<Player> m_player;
};
//! [1]
