/*
 * Copyright (c) 2017-2019 WangBin <wbsecg1 at gmail.com>
 */
#include <cassert>
#include <cstring>
extern "C" {
#include<GL/glx.h>
}
#include "mdk/Player.h"
using namespace MDK_NS;

int main(int argc, char** argv)
{
    const char* cv = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-c:v") == 0) { // VideoToolbox 0-copy rendering works for GLX(macOS)
            cv = argv[++i];
        }
    }

    Display *dpy = XOpenDisplay(nullptr);// XOpenDisplay(NULL);
    Window root = DefaultRootWindow(dpy);
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
    assert(vi);
    XSetWindowAttributes swa;
    swa.colormap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    swa.event_mask = ExposureMask | KeyPressMask;
    Window win = XCreateWindow(dpy, root, 0, 0, 800, 450, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    XMapWindow(dpy, win);
    XStoreName(dpy, win, "MDK Player on X11 Window");

    MDK_NS::Player player;
    if (cv)
        player.setDecoders(MediaType::Video, {cv});
    player.updateNativeSurface((void*)win, -1, -1, MDK_NS::Player::SurfaceType::X11);
    player.setVideoSurfaceSize(800, 450);
    player.setMedia(argv[argc-1]);
    player.set(MDK_NS::State::Playing);
    while (true) {
        XEvent xev;
        XNextEvent(dpy, &xev);
        if(xev.type == Expose) {
            XWindowAttributes gwa;
            XGetWindowAttributes(dpy, win, &gwa);
            player.setVideoSurfaceSize((int)gwa.width, (int)gwa.height);
        } else if(xev.type == KeyPress) {
            XDestroyWindow(dpy, win);
            XCloseDisplay(dpy);
            return 0;
        } else if (xev.type == ConfigureNotify) {
            player.setVideoSurfaceSize(xev.xconfigure.width, xev.xconfigure.height);
        }
    }
}
