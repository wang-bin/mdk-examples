/*
 * Copyright (c) 2018-2023 WangBin <wbsecg1 at gmail.com>
 */
#ifndef QMDKRenderer_H
#define QMDKRenderer_H

#include <QOpenGLWindow>
#include <memory>

namespace mdk {
class Player;
}
class QMDKPlayer;
#ifndef Q_MDK_API
#define Q_MDK_API Q_DECL_IMPORT
#endif
class Q_MDK_API QMDKWindowRenderer : public QOpenGLWindow
{
    Q_OBJECT
public:
    QMDKWindowRenderer(QWindow *parent = nullptr);
    ~QMDKWindowRenderer() override;
    void setSource(QMDKPlayer* player);
    void setROI(const float* videoRoi, const float* viewportRoi = nullptr);

protected:
    virtual void beforeGL() {}
    virtual void afterGL() {}

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
private:
    std::shared_ptr<QMDKPlayer> player_;
};

#ifdef QT_WIDGETS_LIB
#include <QOpenGLWidget>
class Q_MDK_API QMDKWidgetRenderer : public QOpenGLWidget
{
    Q_OBJECT
public:
    QMDKWidgetRenderer(QWidget *parent = nullptr);
    ~QMDKWidgetRenderer() override;
    void setSource(QMDKPlayer* player);
    QMDKPlayer* source() const { return player_; }
    void setROI(const float* videoRoi, const float* viewportRoi = nullptr);
protected:
    virtual void beforeGL() {}
    virtual void afterGL() {}

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
private:
    QMDKPlayer* player_ = nullptr;
};
#endif // QT_GUI_LIB
#endif // QMDKRenderer_H
