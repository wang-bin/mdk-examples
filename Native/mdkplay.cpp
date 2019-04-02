/*
 * Copyright (c) 2016-2019 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with internally created window example
 */
#include <cstring>
#include <mdk/Player.h>
using namespace MDK_NS;
using namespace std;
int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("usage: %s [-c:v decoder] [-size widthxheight] url\n", argv[0]);
        return 0;
    }
    Player p;
    p.createSurface(); // windows, x11, wayland, gbm, rpi 
    p.resizeSurface(1280, 720);
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-c:v") == 0) {
            p.setVideoDecoders({argv[++i]});
        } else if(std::strcmp(argv[i], "-size") == 0) {
            int w = 1920, h=1080;
            sscanf(argv[++i], "%dx%d", &w, &h);
            p.resizeSurface(w, h); 
        }
    }
    p.setMedia(argv[argc-1]);
    p.setState(State::Playing);
    p.showSurface();
    return 0;
}
