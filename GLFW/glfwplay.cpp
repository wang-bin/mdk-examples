
/*
 * Copyright (c) 2016-2019 WangBin <wbsecg1 at gmail.com>
 * MDK SDK + GLFW example
 */
#include "mdk/cpp/Player.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <regex>
#include <GLFW/glfw3.h>
#if _WIN32
#define putenv _putenv // putenv_s(name, value).  for getenv(), can not use SetEnvironmentVariable
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

static void* sNativeDisp = nullptr;
extern "C" MDK_EXPORT void* GetCurrentNativeDisplay() // required by vdpau/vaapi interop with x11 egl if gl context is provided by user because x11 can not query the fucking Display* via the Window shit. not sure about other linux ws e.g. wayland
{
    return sNativeDisp;
}

void setNativeDisplay(void* disp)
{
    sNativeDisp = disp;
}

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
        case GLFW_KEY_UP:
            p->setPlaybackRate(p->playbackRate() + 0.1f);
            break;
        case GLFW_KEY_DOWN:
            p->setPlaybackRate(p->playbackRate() - 0.1f);
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
        case GLFW_KEY_O:
            static int angle = 0;
            p->rotate(angle+=90);
            break;
        case GLFW_KEY_Q:
            glfwSetWindowShouldClose(win, 1);
            break;
        default:
            break;
    }
}

void showHelp(const char* argv0)
{
    printf("usage: %s [-es] [-fps int_fps] [-c:v decoder] url1 [url2 ...]\n"
            "-es: use OpenGL ES2+ instead of OpenGL\n"
            "-c:v: video decoder names separated by ','. can be FFmpeg, VideoToolbox, MFT, D3D11, DXVA, NVDEC, CUDA, VDPAU, VAAPI, MMAL(raspberry pi), CedarX(sunxi), MediaCodec\n"
            "a decoder can set property in format 'name:key1=value1:key2=value2'. for example, VideoToolbox:glva=1:hwdec_format=nv12, MFT:d3d=11:pool=1\n"
            "decoder properties(not supported by all): glva=0/1, hwdec_format=..., copy_frame=0/1/-1, hwdevice=...\n"
            "-c:a: audio decoder names separated by ','. can be FFmpeg, MediaCodec. Properties: hwaccel=avcodec_codec_name_suffix(for example at,fixed)\n"
            "-ao: audio renderer. OpenAL, DSound, XAudio2, ALSA, AudioQueue\n"
            "-url:a: individual audio track url."
            "-from: start from given seconds\n"
            "-gfxthread: create gfx(rendering) context and thread by mdk instead of GLFW. -fps does not work\n"
            "-buffer: buffer duration range in milliseconds, can be 'minMs', 'minMs+maxMs', e.g. -buffer 1000, or -buffer 1000+2000"
            "-buffer_drop: drop buffered data when buffered duration exceed max buffer duration. useful for playing realtime streams, e.g. -buffer 0+1000 -buffer_drop to ensure delay < 1s"
            "-autoclose: close when stopped" // TODO: check image or video
            "Keys:\n"
            "space: pause/resume\n"
            "left/right: seek backward/forward (-/+10s)\n"
            "right: seek forward (+10s)\n"
            "up/down: speed +/-0.1\n"
            "F: fullscreen\n"
            "Q: quit\n"
            // TODO: display env vars
        , argv0);
}

int url_now = 0;
std::vector<std::string> urls;
int main(int argc, char** argv)
{
    bool help = argc < 2;
    bool es = false;
    bool gfxthread = false;
    bool autoclose = false;
    int from = 0;
    float wait = 0;
    int64_t buf_min = 4000;
    int64_t buf_max = 16000;
    bool buf_drop = false;
    const char* urla = nullptr;
    std::string ca, cv;
    Player player;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c:v") == 0) {
            cv = argv[++i];
        } else if (strcmp(argv[i], "-c:a") == 0) {
            ca = argv[++i];
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
        } else if (std::strcmp(argv[i], "-url:a") == 0) {
            urla = argv[++i];
        } else if (std::strcmp(argv[i], "-buffer") == 0) {
            char *s = argv[++i];
            buf_min = strtoll(s, &s, 10);
            if (s[0])
                buf_max = strtoll(s, nullptr, 10);
        } else if (std::strcmp(argv[i], "-buffer_drop") == 0) {
            buf_drop = true;
        } else if (std::strcmp(argv[i], "-autoclose") == 0) {
            autoclose = true;
        } else if (argv[i][0] == '-') {
            printf("Unknow option: %s\n", argv[i]);
            help = true;
        } else {
            for (int j = i; j < argc; ++j)
                urls.emplace_back(argv[j]);
            break;
        }
    }
    if (help)
        showHelp(argv[0]);
    if ((buf_min >= 0 && buf_max >= 0) || buf_drop)
        player.setBufferRange(buf_min, buf_max, buf_drop);
    player.currentMediaChanged([&]{
        std::printf("currentMediaChanged %d/%d, now: %s\n", url_now, urls.size(), player.url());fflush(stdout);
        if (urls.size() > url_now+1) {
            player.setNextMedia(urls[++url_now].data()); // safe to use ref to player
            // alternatively, you can create a custom event
        }
    });
    player.onMediaStatusChanged([](MediaStatus s){
        //MediaStatus s = player.mediaStatus();
        printf("************Media status: %#x, loading: %d, buffering: %d, prepared: %d, EOF: %d**********\n", s, s&MediaStatus::Loading, s&MediaStatus::Buffering, s&MediaStatus::Prepared, s&MediaStatus::End);
        fflush(stdout);
        return true;
    });
    player.onEvent([](const MediaEvent& e){
        printf("MediaEvent: %s %s %" PRId64 "......\n", e.category.data(), e.detail.data(), e.error);
        return false;
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
    GLFWwindow *win = glfwCreateWindow(w, h, "MDK + GLFW. Drop videos here", nullptr, nullptr);
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
    glfwSetScrollCallback(win, [](GLFWwindow* win, double dx, double dy){
        //std::printf("scroll: %.03f, %.04f\n", dx, dy);fflush(stdout);
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        static float s = 1.0f;
        s += dy/5.0f;
        p->scale(s, s);
    });
    glfwSetDropCallback(win, [](GLFWwindow* win, int count, const char** files){
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        p->setState(State::Stopped);
        p->waitFor(State::Stopped);
        urls.clear();
        for (int i = 0; i < count; ++i)
            urls.emplace_back(files[i]);
        url_now = 0;
        p->setMedia(nullptr); // 1st url may be the same as current url
        p->setMedia(urls[url_now].data());
        p->setState(State::Playing);
    });
    glfwShowWindow(win);
#if defined(GLFW_EXPOSE_NATIVE_X11)
    setNativeDisplay(glfwGetX11Display());
#endif
    player.onStateChanged([=](State s){
        if (s == State::Stopped && autoclose) {
            glfwSetWindowShouldClose(win, 1);
            glfwPostEmptyEvent();
        }
    });
    player.setPreloadImmediately(false); // MUST set before setMedia() because setNextMedia() is called when media is changed
    player.setVideoSurfaceSize(w, h);
    //player.setPlaybackRate(2.0f);
    if (!urls.empty())
        player.setMedia(urls[url_now].data());
    if (urla) {
        player.setMedia(urla, MediaType::Audio);
    }
    if (!cv.empty()) {
        std::regex re(",");
        std::sregex_token_iterator first{cv.begin(), cv.end(), re, -1}, last;
        player.setVideoDecoders({first, last});
    }
    if (!ca.empty()) {
        std::regex re(",");
        std::sregex_token_iterator first{ca.begin(), ca.end(), re, -1}, last;
        player.setAudioDecoders({first, last});
    }

    player.prepare(from*1000LL, [](int64_t t, bool*) {
        std::clog << ">>>>>>>>>>>>>>>>>prepared @" << t << std::endl; // FIXME: t is wrong http://qthttp.apple.com.edgesuite.net/1010qwoeiuryfg/sl.m3u8
    });
    player.setState(State::Playing);

    if (gfxthread) {
        auto surface_type = MDK_NS::Player::SurfaceType::Auto;
        putenv((char*)std::string("GL_EGL=").append(std::to_string(es)).data()); // for getenv()
#if defined(_WIN32)
        auto hwnd = glfwGetWin32Window(win);
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
        auto hwnd = glfwGetCocoaWindow(win);
#elif defined(GLFW_EXPOSE_NATIVE_X11)
        auto hwnd = glfwGetX11Window(win);
        surface_type = MDK_NS::Player::SurfaceType::X11;
#endif
        player.updateNativeWindow((void*)hwnd, -1, -1, surface_type);
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
    Player::foreignGLContextDestroyed();
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
