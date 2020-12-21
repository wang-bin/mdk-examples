
/*
 * Copyright (c) 2016-2019 WangBin <wbsecg1 at gmail.com>
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
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif
#include <GLFW/glfw3native.h>

using namespace MDK_NS;

static void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS)
        return;
    auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
    switch (key) {
        case GLFW_KEY_SPACE:
            p->setState(p->state() == State::Playing ? State::Paused : State::Playing);
            break;
        case GLFW_KEY_RIGHT:
            p->seek(p->position()+10000);
            break;
        case GLFW_KEY_LEFT:
            p->seek(p->position()-10000);
            break;
        case GLFW_KEY_F: {
            if (glfwGetWindowMonitor(win)) {
                glfwSetWindowMonitor(win, nullptr, 0, 0, 1920, 1080, 60);
                glfwMaximizeWindow(win);
            } else {
                glfwSetWindowMonitor(win, glfwGetPrimaryMonitor(), 0, 0, 1920, 1080, 60);
            }
            break;
        }
        case GLFW_KEY_Q:
            glfwDestroyWindow(win);
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
            "-gfxthread: create gfx(rendering) context and thread by mdk instead of GLFW. -fps does not work\n"
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
    Player player;
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
        printf("************Media status: %#x, loading: %d, buffering: %d, prepared: %d, EOF: %d**********\n", s, s&MediaStatus::Loading, s& MediaStatus::Buffering, s& MediaStatus::Prepared, s& MediaStatus::End);
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
    glfwSetWindowUserPointer(win, &player);
    glfwSetKeyCallback(win, key_callback);

    glfwSetWindowSizeCallback(win, [](GLFWwindow* win, int w,int h){
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        p->setVideoSurfaceSize(w, h);
    });
    glfwShowWindow(win);


    player.setVideoSurfaceSize(w, h);
    player.setMedia(argv[url_now]);
    //player.setPreloadImmediately(false);
    player.prepare(from*1000LL, [](int64_t t, bool*) {
        std::clog << ">>>>>>>>>>>>>>>>>prepared @" << t << std::endl; // FIXME: t is wrong http://qthttp.apple.com.edgesuite.net/1010qwoeiuryfg/sl.m3u8
#if defined(MDK_VERSION_CHECK)
# if MDK_VERSION_CHECK(0, 5, 0)
        return true;
# endif
#endif
    });
    player.setState(State::Playing);

    if (gfxthread) {
#if defined(_WIN32)
        _putenv(std::string("GL_EGL=").append(std::to_string(es)).data()); // for getenv(), can not use SetEnvironmentVariable. _putenv_s(name, value)
        auto hwnd = glfwGetWin32Window(win);
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
        putenv((char*)std::string("GL_EGL=").append(std::to_string(es)).data()); // for getenv()
        auto hwnd = glfwGetCocoaWindow(win);
#endif
        player.updateNativeSurface(hwnd);
        //player.showWindow(); // let glfw process events. event handling in mdk is only implemented in win32 and x11 for now
        //exit(EXIT_SUCCESS);
    }

    while (!glfwWindowShouldClose(win)) {
        if (!gfxthread) {
            glfwMakeContextCurrent(win);
            player.renderVideo();
            glfwSwapBuffers(win);
        }
        //glfwWaitEventsTimeout(0.04); // if player.setRenderCallback() is not called, render at 25fps
        if (wait > 0)
            glfwWaitEventsTimeout(wait);
        else
            glfwWaitEvents();
    }
    player.setVideoSurfaceSize(-1, -1); // cleanup gl renderer resources in current context
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
