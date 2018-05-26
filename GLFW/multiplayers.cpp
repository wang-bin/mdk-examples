/*
 * Copyright (c) 2018 WangBin <wbsecg1 at gmail.com>
 * MDK SDK example of multiple players
 */
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstring>
#include <vector>
#include "mdk/Player.h"
using namespace MDK_NS;

int main(int argc, char** argv)
{
    printf("MDK + GLFW players\n");
    if (argc < 2) {
        printf("usage: %s [-share] [-fps int_fps] [-win win_count] [-c:v decoder] [-urls] video_url\n", argv[0]);
        printf("-win: windows count, applied if no -urls option, assume only 1 url\n"
            "-urls: the number of windows is url count\n");
        exit(EXIT_FAILURE);
    }
    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "GLFW Error: %s\n", description);
    });
    if (!glfwInit())
        exit(EXIT_FAILURE);
    auto monitor = glfwGetPrimaryMonitor();
    auto mode = glfwGetVideoMode(monitor);
    printf("primary screen size: %dx%d\n", mode->width, mode->height);
    int nb_win = 4;
    float wait = 0;
    bool share = false;
    bool es = false;
    const char* dec = nullptr;
    char* const* urls = nullptr;
    int urls_idx = 0;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-win") == 0) {
            nb_win = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-c:v") == 0) {
            dec = argv[++i];
        } else if (strcmp(argv[i], "-fps") == 0) {
            wait = 1.0f/atoi(argv[++i]);
        } else if (strcmp(argv[i], "-share") == 0) {
            share = true;
        } else if (strcmp(argv[i], "-es") == 0) {
            es = true;
        } else if (strcmp(argv[i], "-urls") == 0) {
            urls_idx = ++i;
        }
    }
    if (urls_idx == 0)
        urls_idx = argc-1;
    urls = &argv[urls_idx];
    const int nb_urls = argc - urls_idx;
    if (nb_urls > 1)
        nb_win = nb_urls;
    const int dim = (int)ceil(sqrt(nb_win));
    const int w =  mode->width/dim, h =  mode->height/dim;
    std::vector<GLFWwindow*> win(nb_win);
    std::vector<std::unique_ptr<Player>> players(nb_win);
    for (int i = 0;  i < nb_win;  i++) {
        auto& player = players[i];
        player.reset(new Player());
        if (dec)
            player->setVideoDecoders({dec});
    if (wait <= 0)
        player->setRenderCallback(glfwPostEmptyEvent);
    player->setMedia(urls[nb_urls > 1 ? i : 0]);
    player->setState(State::Playing);
    //player->setActiveTracks(MediaType::Audio, {});
#if 0
// test different context profiles and versions
        if (i >= 2 && !share) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        }
#endif
        if (es) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        }
        player->setVideoSurfaceSize(w, h);
        win[i] = glfwCreateWindow(w, h, "MDK + GLFW multi-window", nullptr, share ? win[0] : nullptr);
        if (!win[i]) {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        glfwSetKeyCallback(win[i], [](GLFWwindow* window, int key, int scancode, int action, int mods){
            if (action != GLFW_PRESS)
                return;
            switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            }
        });
        int left, top, right, bottom;
        glfwGetWindowFrameSize(win[i], &left, &top, &right, &bottom);
        const int row = i/dim;
        const int col = i%dim;
        glfwSetWindowPos(win[i], 60 + col * (w + left + right), 60 + row * (h + top + bottom));
        glfwShowWindow(win[i]);
    }
    while (true) {
        for (int i = 0; i < nb_win; ++i) {
            auto& w = win[i];
            auto& player = players[i];
            glfwMakeContextCurrent(w);
            player->renderVideo();
            glfwSwapBuffers(w);
            if (glfwWindowShouldClose(w))
                goto end;
        }
        if (wait > 0)
            glfwWaitEventsTimeout(wait);
        else
            glfwWaitEvents();
    }
end:
    glfwTerminate();
    exit(EXIT_SUCCESS);
}