/*
 * Copyright (c) 2016-2018 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWindow example
 */
#ifndef QMDKWINDOW_H
#define QMDKWINDOW_H

#include <QOpenGLWindow>
#include <memory>

namespace mdk {
class Player;
}
class QMDKWindow : public QOpenGLWindow
{
public:
    QMDKWindow(QWindow *parent = Q_NULLPTR);
    ~QMDKWindow();
    void setDecoders(const QStringList& dec);
    void setMedia(const QString& url);
    void play();
    void pause();
    void stop();
    bool isPaused() const;
    void seek(qint64 ms);
    qint64 position() const;
    void capture(QObject* vo);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent *) override;
private:
    std::shared_ptr<mdk::Player> player_;
};

#endif // QMDKWINDOW_H
