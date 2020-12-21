/*
 * Copyright (c) 2020 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWindow example
 */
#pragma once
#include <QOpenGLWidget>
#include <memory>

namespace mdk {
class Player;
}
class QMDKWidget : public QOpenGLWidget
{
public:
    QMDKWidget(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~QMDKWidget();
    void setDecoders(const QStringList& dec);
    void setMedia(const QString& url);
    void play();
    void pause();
    void stop();
    bool isPaused() const;
    void seek(qint64 ms);
    qint64 position() const;
    void snapshot();

protected:
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent *) override;
private:
    std::shared_ptr<mdk::Player> player_;
};
