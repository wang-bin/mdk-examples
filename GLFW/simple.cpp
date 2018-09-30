
/*
 * Copyright (c) 2016-2018 WangBin <wbsecg1 at gmail.com>
 * MDK SDK + GLFW example
 */
#include "mdk/Player.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <GLFW/glfw3.h>
#if _WIN32
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

using namespace MDK_NS;

Player player;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS)
        return;
    switch (key) {
        case GLFW_KEY_SPACE:
            player.setState(player.state() == State::Playing ? State::Paused : State::Playing);
            break;  
        case GLFW_KEY_RIGHT:
            player.seek(player.position()+10000);
            break;
        case GLFW_KEY_LEFT:
            player.seek(player.position()-10000);
            break;
        case GLFW_KEY_F: {
            if (glfwGetWindowMonitor(window)) {
                glfwSetWindowMonitor(window, nullptr, 0, 0, 1920, 1080, 60);
                glfwMaximizeWindow(window);
            } else {
                glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, 1920, 1080, 60);
            }
            break;
        }
        case GLFW_KEY_Q:
            glfwDestroyWindow(window);
            break;
        default:
            break;
    }
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("usage: %s [-es] [-fps int_fps] [-c:v decoder] url1 [url2 ...]\n"
            "-es: use OpenGL ES2+ instead of OpenGL\n"
            "-c:v: video decoder. can be FFmpeg, VideoToolbox, D3D11, DXVA, NVDEC, CUDA, VDPAU, VAAPI, MMAL(raspberry pi), CedarX(sunxi)\n"
            "-ao: audio renderer. OpenAL, DSound, XAudio2, ALSA, AudioQueue\n"
            "-from: start from given seconds\n"
            "-gfxthread: create gfx(rendering) context and thread by mdk instead of GLFW. -fps does not work"
            // TODO: display env vars
        , argv[0]);
        exit(EXIT_FAILURE);
    }
    bool es = false;
    bool gfxthread = false;
    int url_idx = 0;
    int url_now = 0;
    int from = 0;
    float wait = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c:v") == 0) {
            player.setVideoDecoders({argv[++i]});
        } else if (strcmp(argv[i], "-es") == 0) {
            es = true;
        } else if (std::strcmp(argv[i], "-from") == 0) {
            from = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-ao") == 0) {
            player.setAudioBackends({argv[++i]});
        } else if (std::strcmp(argv[i], "-fps") == 0) {
            wait = 1.0f/atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-gfxthread") == 0) {
            gfxthread = true;
        } else {
            url_now = url_idx = i;
            break;
        }
    }

    player.currentMediaChanged([&]{
        printf("currentMediaChanged %d/%d, now: %s\n", url_now-url_idx, argc-url_idx, player.url());fflush(stdout);
        if (argc > url_now+1) {
            player.setNextMedia(argv[++url_now]); // safe to use ref to player
            // alternatively, you can create a custom event
        }
    });
    player.onMediaStatusChanged([](MediaStatus s){
        //MediaStatus s = player.mediaStatus();
        printf("************Media status: %#x, loading: %d, buffering: %d, prepared: %d, EOF: %d**********\n", s, s&LoadingMedia, s&BufferingMedia, s&PreparedMedia, s&EndOfMedia);
        return true;
    });
    if (!gfxthread && wait <= 0)
        player.setRenderCallback([](void*){
            glfwPostEmptyEvent();
        });

    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "Error: %s\n", description);
    });
    if (!glfwInit())
        exit(EXIT_FAILURE);
    if (es && !gfxthread) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    }
    const int w = 640, h = 480;
    GLFWwindow *win = glfwCreateWindow(w, h, "MDK + GLFW", nullptr, nullptr);
    if (!win) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetKeyCallback(win, key_callback);

    glfwSetWindowSizeCallback(win, [](GLFWwindow*, int w,int h){
        player.setVideoSurfaceSize(w, h);
    });
    glfwShowWindow(win);


    player.setVideoSurfaceSize(w, h);
    player.setMedia(argv[url_now]);
    //player.setPreloadImmediately(false);
    player.prepare(from*1000LL, [](int64_t t, bool*) {
        std::clog << ">>>>>>>>>>>>>>>>>prepared @" << t << std::endl; // FIXME: t is wrong http://qthttp.apple.com.edgesuite.net/1010qwoeiuryfg/sl.m3u8
    });
    player.setState(State::Playing);

#if _WIN32
    if (gfxthread) {
        _putenv(std::string("GL_EGL=").append(std::to_string(es)).data()); // for getenv()
        auto hwnd = glfwGetWin32Window(win);
        player.updateNativeWindow(hwnd);
        player.showWindow();
        std::clog << "exiting..." << std::endl;
        exit(EXIT_SUCCESS);
    }
#endif

    while (!glfwWindowShouldClose(win)) {
        glfwMakeContextCurrent(win);
        player.renderVideo();
        glfwSwapBuffers(win);
        //glfwWaitEventsTimeout(0.04); // if player.setRenderCallback() is not called, render at 25fps
        if (wait > 0)
            glfwWaitEventsTimeout(wait);
        else
            glfwWaitEvents();
    }
    Player::foreignGLContextDestroyed();
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
