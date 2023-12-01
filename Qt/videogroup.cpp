/******************************************************************************
    VideoGroup:  this file is part of QtAV examples
    Copyright (C) 2012-2023 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "videogroup.h"
#include <QApplication>
#include <QEvent>
#include <QFileDialog>
#include <QGridLayout>
#include <QScreen>
#include "QMDKRenderer.h"
using namespace std;
VideoGroup::VideoGroup(QObject *parent) :
    QObject(parent)
  , m1Window(false)
  , m1Frame(true)
  , mFrameless(false)
  , r(3),c(3)
  , vid(QString::fromLatin1("qpainter"))
{
    mpPlayer = new QMDKPlayer(this);
    int i = qApp->arguments().indexOf("-c:v");
    if (i > 0)
        mpPlayer->setDecoders(qApp->arguments().at(i+1).split(","));
    mpBar = new QWidget(0, Qt::WindowStaysOnTopHint);
    mpBar->setMaximumSize(400, 60);
    mpBar->show();
    mpBar->setLayout(new QHBoxLayout);
    mpOpen = new QPushButton(tr("Open"));
    mpPlay = new QPushButton(tr("Play"));
    mpStop = new QPushButton(tr("Stop"));
    mpPause = new QPushButton(tr("Pause"));
    mpPause->setCheckable(true);
    mpAdd = new QPushButton(QString::fromLatin1("+"));
    mpRemove = new QPushButton(QString::fromLatin1("-"));
    mp1Window = new QPushButton(tr("Single Window"));
    mp1Window->setCheckable(true);
    mp1Frame = new QPushButton(tr("Single Frame"));
    mp1Frame->setCheckable(true);
    mp1Frame->setChecked(true);
    mpFrameless = new QPushButton(tr("no window frame"));
    mpFrameless->setCheckable(true);
    connect(mpOpen, SIGNAL(clicked()), SLOT(openLocalFile()));
    connect(mpPlay, SIGNAL(clicked()), mpPlayer, SLOT(play()));
    connect(mpStop, SIGNAL(clicked()), mpPlayer, SLOT(stop()));
    connect(mpPause, SIGNAL(toggled(bool)), mpPlayer, SLOT(pause(bool)));
    connect(mpAdd, SIGNAL(clicked()), SLOT(addRenderer()));
    connect(mpRemove, SIGNAL(clicked()), SLOT(removeRenderer()));
    connect(mp1Window, SIGNAL(toggled(bool)), SLOT(setSingleWindow(bool)));
    connect(mp1Frame, SIGNAL(toggled(bool)), SLOT(toggleSingleFrame(bool)));
    connect(mpFrameless, SIGNAL(toggled(bool)), SLOT(toggleFrameless(bool)));

    mpBar->layout()->addWidget(mpOpen);
    mpBar->layout()->addWidget(mpPlay);
    mpBar->layout()->addWidget(mpStop);
    mpBar->layout()->addWidget(mpPause);
    mpBar->layout()->addWidget(mpAdd);
    mpBar->layout()->addWidget(mpRemove);
    mpBar->layout()->addWidget(mp1Window);
    mpBar->layout()->addWidget(mp1Frame);
    //mpBar->layout()->addWidget(mpFrameless);
    mpBar->resize(200, 25);
}

VideoGroup::~VideoGroup()
{
    delete mpBar;
}

void VideoGroup::setSingleWindow(bool s)
{
    m1Window = s;
    if (mRenderers.isEmpty())
        return;
    if (!s) {
        if (!view)
            return;
        for (auto vo : mRenderers) {
            view->layout()->removeWidget(vo);
            vo->setParent(nullptr);
            auto wf = vo->windowFlags();
            if (mFrameless) {
                wf &= ~Qt::FramelessWindowHint;
            } else {
                wf |= Qt::FramelessWindowHint;
            }
            vo->setWindowFlags(wf);
            vo->show();
        }
        view.reset();
    } else {
        if (view)
            return;
        view.reset(new QWidget);
        view->resize(view->screen()->size());
        QGridLayout *layout = new QGridLayout;
        layout->setSizeConstraint(QLayout::SetMaximumSize);
        layout->setSpacing(1);
        layout->setContentsMargins(0, 0, 0, 0);
        view->setLayout(layout);

        for (int i = 0; i < mRenderers.size(); ++i) {
            int x = i/c; // row
            int y = i%c; // col
            ((QGridLayout*)view->layout())->addWidget(mRenderers.at(i), x, y);
        }
        view->show();
        mRenderers.last()->showFullScreen();
    }
}

void VideoGroup::toggleSingleFrame(bool s)
{
    if (m1Frame == s)
        return;
    m1Frame = s;
    updateROI();
}

void VideoGroup::toggleFrameless(bool f)
{
    mFrameless = f;
    if (mRenderers.isEmpty())
        return;
    auto wf = mRenderers.first()->windowFlags();
    if (f) {
        wf &= ~Qt::FramelessWindowHint;
    } else {
        wf |= Qt::FramelessWindowHint;
    }
    for (auto w : mRenderers)
        w->setWindowFlags(wf);
}

void VideoGroup::setRows(int n)
{
    r = n;
}

void VideoGroup::setCols(int n)
{
    c = n;
}

void VideoGroup::show()
{
    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < c; ++j) {
            addRenderer();
        }
    }
}

void VideoGroup::play(const QString &file)
{
    mpPlayer->setMedia(file);
    mpPlayer->play();
}

void VideoGroup::openLocalFile()
{
    QString file = QFileDialog::getOpenFileName(0, tr("Open a video"));
    if (file.isEmpty())
        return;
    mpPlayer->setMedia(file);
    mpPlayer->play();
}

void VideoGroup::addRenderer()
{
    auto v = new QMDKWidgetRenderer();
    mRenderers.append(v);
    v->setAttribute(Qt::WA_DeleteOnClose);
    auto wf = v->windowFlags();
    if (mFrameless) {
        wf &= ~Qt::FramelessWindowHint;
    } else {
        wf |= Qt::FramelessWindowHint;
    }
    v->setWindowFlags(wf);
    int w = view ? view->frameGeometry().width()/c : qApp->primaryScreen()->size().width()/c;
    int h = view ? view->frameGeometry().height()/r : qApp->primaryScreen()->size().height()/r;
    v->resize(w, h);
    v->setSource(mpPlayer);
    int i = (mRenderers.size()-1)/c;
    int j = (mRenderers.size()-1)%c;
    if (view) {
        view->resize(view->screen()->size());
        ((QGridLayout*)view->layout())->addWidget(v, i, j);
        view->show();
    } else {
        v->move(j*w, i*h);
        v->show();
    }
    updateROI();
}

void VideoGroup::removeRenderer()
{
    if (mRenderers.isEmpty())
        return;
    auto r = mRenderers.takeLast();
    if (view) {
        view->layout()->removeWidget(r);
    }
    delete r;
    updateROI();
}

void VideoGroup::updateROI()
{
    if (mRenderers.isEmpty())
        return;
    if (!m1Frame) {
        for (auto v : mRenderers) {
            qobject_cast<QMDKWidgetRenderer*>(v)->setROI(nullptr);
        }
        return;
    }

    for (int i = 0; i < mRenderers.size(); ++i) {
        auto v = mRenderers.at(i);
        int x = i/c; // row
        int y = i%c; // col
        float videoRoi[] = { float(y)/float(c), float(x)/float(r), float(y + 1)/float(c), float(x + 1)/float(r) };
        qobject_cast<QMDKWidgetRenderer*>(v)->setROI(videoRoi);
    }
}
