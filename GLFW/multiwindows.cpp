/*
 * Copyright (c) 2016-2020 WangBin <wbsecg1 at gmail.com>
 * MDK SDK example of rendering 1 video in multiple windows and OpenGL contexts
 */
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstring>
#include <vector>
#include "mdk/Player.h"
using namespace MDK_NS;

int main(int argc, char** argv)
{
    printf("MDK + GLFW multi-window and multi-context\n");
    if (argc < 2) {
        printf("usage: %s [-es] [-share] [-fps int_fps] [-win win_count] [-c:v decoder] video_url\n"
            "-es: use OpenGL ES2+ instead of OpenGL\n"
            "-share: shared OpenGL/ES contexts\n"
            "-win: number of windows\n"
            "-c:v: video decoder. can be FFmpeg, VideoToolbox, D3D11, DXVA, NVDEC, CUDA, VDPAU, VAAPI\n"
        , argv[0]);
        exit(EXIT_FAILURE);
    }
    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "GLFW Error: %s\n", description);
    });
    if (!glfwInit())
        exit(EXIT_FAILURE);
{
    auto monitor = glfwGetPrimaryMonitor();
    auto mode = glfwGetVideoMode(monitor);
    printf("primary screen size: %dx%d\n", mode->width, mode->height);
    Player player;
    int nb_win = 4;
    float wait = 0;
    bool share = false;
    bool es = false;
    bool merge = false; // merge all windows into 1 frame
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-win") == 0) {
            nb_win = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-c:v") == 0) {
            player.setDecoders(MediaType::Video, {argv[++i]});
        } else if (strcmp(argv[i], "-fps") == 0) {
            wait = 1.0f/atoi(argv[++i]);
        } else if (strcmp(argv[i], "-share") == 0) {
            share = true;
        } else if (strcmp(argv[i], "-es") == 0) {
            es = true;
        } else if (strcmp(argv[i], "-merge") == 0) {
            merge = true;
        }
    }
    if (wait <= 0)
        player.setRenderCallback([](void*){
            glfwPostEmptyEvent();
        });
    player.setMedia(argv[argc-1]);
    player.set(State::Playing);

    const int dim = (int)ceil(sqrt(nb_win));
    const int w =  mode->width/dim, h =  mode->height/dim;
    std::vector<GLFWwindow*> win(nb_win);
    for (int i = 0;  i < nb_win;  i++) {
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
        win[i] = glfwCreateWindow(w, h, "MDK + GLFW multi-window", nullptr, share ? win[0] : nullptr);
        player.setVideoSurfaceSize(w, h, win[i]);
        //if (!merge) player.setVideoSurfaceSize(w, h); // this works too. unlike renderVideo in a callback, no need to set vo opaque because every window is updated
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
        for (auto w : win) {
            glfwMakeContextCurrent(w);
            player.renderVideo(w);
            // player.renderVideo(); // this works too. unlike renderVideo in a callback, no need to set vo opaque because every window is updated
            glfwSwapBuffers(w);
            if (glfwWindowShouldClose(w))
                goto endloop;
        }
        if (wait > 0)
            glfwWaitEventsTimeout(wait);
        else
            glfwWaitEvents();
    }
endloop:
    for (auto w : win) {
        // cleanup renderer and gl resources in render context(current)
        glfwMakeContextCurrent(w);
        player.setVideoSurfaceSize(-1, -1, w);
    }
}
end:
    glfwTerminate();
    exit(EXIT_SUCCESS);
}