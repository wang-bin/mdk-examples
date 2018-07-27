/*
 * Copyright (c) 2018 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWidget example
 */
#ifndef QMDKPlayer_H
#define QMDKPlayer_H

#include <QObject>
#include <memory>
#include <mutex>

namespace mdk {
class Player;
}
#ifndef Q_MDK_API
#define Q_MDK_API Q_DECL_IMPORT
#endif
class Q_MDK_API QMDKPlayer : public QObject
{
public:
    QMDKPlayer(QObject *parent = nullptr);
    ~QMDKPlayer();
    // decoders: "VideoToolbox", "VAAPI", "VDPAU", "D3D11", "DXVA", "NVDEC", "CUDA", "MMAL"(raspberry pi), "CedarX"(sunxi), "MediaCodec", "FFmpeg"
    void setDecoders(const QStringList& dec);
    void setMedia(const QString& url);
    void play();
    void pause();
    void stop();
    bool isPaused() const;
    void seek(qint64 ms);
    qint64 position() const;

    void addRenderer(QObject* vo = nullptr, int w = -1, int h = -1);
    void renderVideo(QObject* vo = nullptr);
    void removeRenderer(QObject* vo = nullptr);
private:
    std::mutex vo_mutex_; // declare mutex first to ensure mutex is destroyed after player because it is used in render callback which may be called in dtor
    std::unique_ptr<mdk::Player> player_;
    std::vector<QObject*> vo_;
};

#endif // QMDKPlayer_H
