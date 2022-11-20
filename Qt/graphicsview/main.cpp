#include "GraphicsVideoItem.h"
#include <QGraphicsView>
#include <QOpenGLWidget>
#include <QCheckBox>
#include <QSlider>
#include <QLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QApplication>

class VideoPlayer : public QWidget
{
public:
    VideoPlayer(QWidget *parent = 0)
        : QWidget(parent)
    {
        videoItem = new GraphicsVideoItem();
        videoItem->resize(640, 480);
        QGraphicsScene *scene = new QGraphicsScene(this);
        scene->addItem(videoItem);

        view = new QGraphicsView(scene);
        view->setViewport(new QOpenGLWidget());
        view->setCacheMode(QGraphicsView::CacheNone);

        QSlider *rotateSlider = new QSlider(Qt::Horizontal);
        rotateSlider->setRange(-180,  180);
        rotateSlider->setValue(0);

        QSlider *scaleSlider = new QSlider(Qt::Horizontal);
        scaleSlider->setRange(0, 200);
        scaleSlider->setValue(100);


        connect(rotateSlider, &QSlider::valueChanged, [=](int angle){
            videoItem->setTransform(QTransform().rotate(angle));
        });
        connect(scaleSlider, &QSlider::valueChanged, [=](int value){
            qreal v = (qreal)value/100.0;
            videoItem->setTransform(QTransform().scale(v, v));
        });

        auto openBtn = new QPushButton();
        openBtn->setText(tr("Open"));
        connect(openBtn, &QPushButton::clicked, [=]{
            QString f = QFileDialog::getOpenFileName(0, tr("Open a video"));
            if (f.isEmpty())
                return;
            videoItem->setMedia(f);
            videoItem->play();
        });


        QHBoxLayout *hb = new QHBoxLayout;
        hb->addWidget(openBtn);
        QBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(view);
        layout->addWidget(rotateSlider);
        layout->addWidget(scaleSlider);
        layout->addLayout(hb);
        setLayout(layout);
    }


    QSize sizeHint() const { return QSize(720, 640); }
    void play(const QString& file) {
        videoItem->setMedia(file);
        videoItem->play();
    }

private:
    GraphicsVideoItem *videoItem = nullptr;
    QGraphicsView *view = nullptr;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    VideoPlayer w;
    w.show();
    if (a.arguments().size() > 1)
        w.play(a.arguments().last());

    return a.exec();
}
