
/*
 * Copyright (c) 2016-2020 WangBin <wbsecg1 at gmail.com>
 * MDK SDK + GLFW example
 */
#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif
#ifdef _WIN32
#include <d3d11.h>
#endif
#include "mdk/Player.h"

#ifndef MDK_VERSION_CHECK
#define MDK_VERSION_CHECK(...) 0
#endif

#if MDK_VERSION_CHECK(0, 5, 0)
#include "mdk/RenderAPI.h"
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <regex>
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
#include "prettylog.h"

#ifdef _Pragma
# if (GLFW_VERSION_MAJOR > 3 ||  GLFW_VERSION_MINOR > 2)
_Pragma("weak glfwSetWindowContentScaleCallback")
_Pragma("weak glfwGetWindowContentScale")
# endif
#endif

using namespace MDK_NS;

int64_t gSeekStep = 10000LL;
SeekFlag gSeekFlag = SeekFlag::Default;

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
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;
    auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
    switch (key) {
        case GLFW_KEY_C: {
            Player::SnapshotRequest req{};
            p->snapshot(&req,
#if MDK_VERSION_CHECK(0, 5, 0)
        [](Player::SnapshotRequest* ret, double frameTime){
            return std::to_string(frameTime).append(".jpg");
#else
        [](Player::SnapshotRequest* ret){
#endif
            });
        }
            break;
        case GLFW_KEY_D: {
            static const char* decs[] = {"FFmpeg", "NullTest",
#if defined(__APPLE__)
                "VT", "VideoToolbox",
#elif defined(_WIN32)
                "MFT:d3d=11", "MFT:d3d=9", "MFT", "D3D11", "DXVA", "CUDA", "NVDEC"
#elif defined(__linux__)
                "VAAPI", "VDPAU", "CUDA", "NVDEC"
#endif
            };
            static int d = 0;
            p->setVideoDecoders({decs[++d%std::size(decs)]});
        }
            break;
        case GLFW_KEY_SPACE:
            p->setState(p->state() == State::Playing ? State::Paused : State::Playing);
            break;
        case GLFW_KEY_RIGHT:
            p->seek(p->position()+gSeekStep, gSeekFlag); // Default if GLFW_REPEAT
            break;
        case GLFW_KEY_LEFT:
            p->seek(p->position()-gSeekStep, gSeekFlag);
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
        case GLFW_KEY_M: {
            static bool value = true;
            p->setMute(value);
            value = !value;
        }
            break;
        case GLFW_KEY_O: {
            static int angle = 0;
            p->rotate(angle+=90);
        }
            break;
        case GLFW_KEY_Q:
            glfwSetWindowShouldClose(win, 1);
            break;
        case GLFW_KEY_R:
            p->record("mdk-record.mkv");
            break;
        case GLFW_KEY_S:
            p->setState(State::Stopped);
            break;
        default:
            break;
    }
}

void showHelp(const char* argv0)
{
    printf("usage: %s [-d3d11] [-es] [-fps int_fps] [-c:v decoder] url1 [url2 ...]\n"
            "-d3d11: d3d11 renderer, MUST use with -gfxthread. support additiona options: -d3d11:feature_level=12.0:debug=1:adapter=0:buffers=2 \n"
            "-es: use OpenGL ES2+ instead of OpenGL\n"
            "-c:v: video decoder names separated by ','. can be FFmpeg, VideoToolbox, MFT, D3D11, DXVA, NVDEC, CUDA, VDPAU, VAAPI, MMAL(raspberry pi), CedarX(sunxi), MediaCodec\n"
            "a decoder can set property in format 'name:key1=value1:key2=value2'. for example, VideoToolbox:glva=1:hwdec_format=nv12, MFT:d3d=11:pool=1\n"
            "decoder properties(not supported by all): glva=0/1, hwdec_format=..., copy_frame=0/1/-1, hwdevice=...\n"
            "-c:a: audio decoder names separated by ','. can be FFmpeg, MediaCodec. Properties: hwaccel=avcodec_codec_name_suffix(for example at,fixed)\n"
            "-ao: audio renderer. OpenAL, DSound, XAudio2, ALSA, AudioQueue\n"
            "-url:a: individual audio track url.\n"
            "-from: start from given seconds\n"
            "-pause: pause at the 1st frame\n"
            "-gfxthread: create gfx(rendering) context and thread by mdk instead of GLFW. -fps does not work\n"
            "-buffer: buffer duration range in milliseconds, can be 'minMs', 'minMs+maxMs', e.g. -buffer 1000, or -buffer 1000+2000\n"
            "-buffer_drop: drop buffered data when buffered duration exceed max buffer duration. useful for playing realtime streams, e.g. -buffer 0+1000 -buffer_drop to ensure delay < 1s\n"
            "-loop-a: A-B loop A. if not set but -loop-b is set, then A is 0\n"
            "-loop-b: A-B loop B. -1 means end of media. if not set but -loop-a is set, then B is -1\n"
            "-loop: A-B loop repeat count\n"
            "-seek_any: seek to any frame instead of key frame\n"
            "-seek_step: step length(in ms) of seeking forward/backward\n"
            "-autoclose: close when stopped\n" // TODO: check image or video
            "Keys:\n"
            "space: pause/resume\n"
            "left/right: seek backward/forward (-/+10s)\n"
            "right: seek forward (+10s)\n"
            "up/down: speed +/-0.1\n"
            "C: capture video frame\n"
#ifdef INCLUDE_STB_IMAGE_WRITE_H
            " and save as N.jpg\n"
#endif
            "\n"
            "D: switch video decoder\n"
            "F: fullscreen\n"
            "L: stop looping\n"
            "O: change orientation\n"
            "Q: quit\n"
            "R: record video as 'mdk-record.mkv', press again to stop recording\n"
            // TODO: display env vars
        , argv0);
}


void parse_options(const char* opts, function<void(const char* opts, const char*)> cb)
{
    if (!opts || !opts[0])
        return;
    while (opts[0] == ':')
        opts++;
    auto options = string(opts);
    char* p = &options[0];
    while (true) {
        char* v = strchr(p, '=');
        *v++ = 0;
        char* pp = strchr(v, ':');
        if (pp)
            *pp = 0;
        cb(p, v);
        if (!pp)
            break;
        p = pp + 1;
    }
}

int url_now = 0;
std::vector<std::string> urls;
int main(int argc, char** argv)
{
    FILE* log_file = stdout;
    setLogHandler([&log_file](LogLevel level, const char* msg){
        static const char level_name[] = {'I', 'W'};
        print_log_msg(level_name[level<LogLevel::Info], msg, log_file);
    });
    SetGlobalOption("MDK_KEY", "0B7B1FB276D89477B7C3FB55B5628E16C71BE63D88989CD1B481E9E746ADA4998797FD4C5DD7EBF6E4710F71354A51BB813A4AB7F15E7DB4802B2EE1B52F6405F484E04D76D89C77483C04AA4A9D71E9C71BEE5AE4FEEBA1D8E090AAC1B0A0CE550AD3E5388B90729D67606737CE7D94889F449C05A309BAEF82629592956305");
{
    bool help = argc < 2;
    bool es = false;
    bool gfxthread = false;
    bool autoclose = false;
    int from = 0;
    float wait = 0;
    float speed = 1.0f;
    int64_t buf_min = 4000;
    int64_t buf_max = 16000;
    int loop = -2;
    int64_t loop_a = -1;
    int64_t loop_b = 0;
    bool buf_drop = false;
    bool pause = false;
    const char* urla = nullptr;
    std::string ca, cv;
    Player player;
#ifdef _WIN32
    D3D11RenderAPI d3d11ra;
#endif
#if (__APPLE__+0) && (MDK_VERSION_CHECK(0, 8, 2) || defined(MDK_ABI))
    MetalRenderAPI mtlra;
#endif
#if MDK_VERSION_CHECK(0, 8, 2) || defined(MDK_ABI)
    GLRenderAPI glra;
#endif
    RenderAPI *ra = nullptr;
    //player.setAspectRatio(-1.0);
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c:v") == 0) {
            cv = argv[++i];
        } else if (strcmp(argv[i], "-c:a") == 0) {
            ca = argv[++i];
        } else if (strstr(argv[i], "-d3d11") == argv[i]) {
            gfxthread = true;
#ifdef _WIN32
            ra = &d3d11ra;
# if MDK_VERSION_CHECK(0, 8, 1) || defined(MDK_ABI)
            parse_options(argv[i] + sizeof("-d3d11") - 1, [&d3d11ra](const char* name, const char* value){
                if (strcmp(name, "debug") == 0)
                    d3d11ra.debug = std::atoi(value);
                else if (strcmp(name, "buffers") == 0)
                    d3d11ra.buffers = std::atoi(value);
                else if (strcmp(name, "adapter") == 0)
                    d3d11ra.adapter = std::atoi(value);
                else if (strcmp(name, "feature_level") == 0)
                    d3d11ra.feature_level = std::atof(value);
            });
# endif
#endif
        } else if (strstr(argv[i], "-gl") == argv[i]) {
            gfxthread = true;
#if MDK_VERSION_CHECK(0, 8, 2) || defined(MDK_ABI)
            ra = &glra;
            parse_options(argv[i] + sizeof("-gl") - 1, [&glra](const char* name, const char* value){
                if (strcmp(name, "debug") == 0)
                    glra.debug = std::atoi(value);
                else if (strcmp(name, "egl") == 0)
                    glra.egl = std::atoi(value);
                else if (strcmp(name, "opengl") == 0)
                    glra.opengl = std::atoi(value);
                else if (strcmp(name, "opengles") == 0)
                    glra.opengles = std::atoi(value);
                else if (strcmp(name, "profile") == 0) {
                    if (strstr(value, "core"))
                        glra.profile = GLRenderAPI::Profile::Core;
                    else if (strstr(value, "compat"))
                        glra.profile = GLRenderAPI::Profile::Compatibility;
                    else
                        glra.profile = GLRenderAPI::Profile::No;
                }
                else if (strcmp(name, "version") == 0)
                    glra.version = std::atof(value);
            });
#endif // MDK_VERSION_CHECK(0, 8, 1) || defined(MDK_ABI)

        } else if (strstr(argv[i], "-metal") == argv[i]) {
            gfxthread = true;
#if (__APPLE__+0) && (MDK_VERSION_CHECK(0, 8, 2) || defined(MDK_ABI))
            ra = &mtlra;
#endif
        } else if (strcmp(argv[i], "-es") == 0) {
            es = true;
        } else if (std::strcmp(argv[i], "-from") == 0) {
            from = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-pause") == 0) {
            pause = true;
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
        } else if (std::strcmp(argv[i], "-loop-a") == 0) {
            loop_a = atoll(argv[++i]);
        } else if (std::strcmp(argv[i], "-loop-b") == 0) {
            loop_b = atoll(argv[++i]);
        } else if (std::strcmp(argv[i], "-loop") == 0) {
            loop = atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-speed") == 0) {
            speed = atof(argv[++i]);
        } else if (std::strcmp(argv[i], "-seek_any") == 0) {
            gSeekFlag = SeekFlag::FromStart;
        } else if (std::strcmp(argv[i], "-seek_step") == 0) {
            gSeekStep = atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-plugins") == 0) {
            SetGlobalOption("plugins", argv[++i]);
        } else if (std::strcmp(argv[i], "-autoclose") == 0) {
            autoclose = true;
        } else if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "-help") == 0) {
            help = true;
            break;
        } else if (std::strcmp(argv[i], "-logfile") == 0) {
            log_file = fopen(argv[++i], "w");
        } else if (argv[i][0] == '-' && (argv[i+1][0] == '-' || i == argc - 2)) {
            printf("Unknow option: %s\n", argv[i]);
        } else if (argv[i][0] == '-') {
            SetGlobalOption(argv[i]+1, argv[i+1]);
            player.setProperty(argv[i]+1, argv[i+1]);
            ++i;
        } else {
            for (int j = i; j < argc; ++j)
                urls.emplace_back(argv[j]);
            break;
        }
    }
    //auto libavformat = dlopen("libavformat.58.dylib", RTLD_LOCAL);
    //SetGlobalOption("avformat", libavformat);
    if (help)
        showHelp(argv[0]);
#if MDK_VERSION_CHECK(0, 5, 0)
    if (ra)
        player.setRenderAPI(ra);
#endif
    if (speed != 1.0f)
        player.setPlaybackRate(speed);
//player.setProperty("continue_at_end", "0");
    //player.setProperty("video.avfilter", "format=pix_fmts=yuv420p10be");
    if ((buf_min >= 0 && buf_max >= 0) || buf_drop)
        player.setBufferRange(buf_min, buf_max, buf_drop);
    player.currentMediaChanged([&]{
        std::printf("currentMediaChanged %d/%zu, now: %s\n", url_now, urls.size(), player.url());fflush(stdout);
        if (urls.size() > url_now+1) {
            player.setNextMedia(urls[++url_now].data()); // safe to use ref to player
            // alternatively, you can create a custom event
        }
    });
    player.onMediaStatusChanged([](MediaStatus s){
        //MediaStatus s = player.mediaStatus();
        printf("************Media status: %#x, invalid: %#x, loading: %d, buffering: %d, seeking: %#x, prepared: %d, EOF: %d**********\n", s, s&MediaStatus::Invalid, s&MediaStatus::Loading, s&MediaStatus::Buffering, s&MediaStatus::Seeking, s&MediaStatus::Prepared, s&MediaStatus::End);
        fflush(stdout);
        return true;
    });
    player.onEvent([](const MediaEvent& e){
        printf("MediaEvent: %s %s %" PRId64 "......\n", e.category.data(), e.detail.data(), e.error);
        return false;
    });
    player.onLoop([](int count){
        printf("++++++++++++++onLoop: %d......\n", count);
        return false;
    });
    if (!gfxthread && wait <= 0)
        player.setRenderCallback([](void*){
            glfwPostEmptyEvent(); // FIXME: some events are lost on macOS. glfw bug?
        });

    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "Error: %s\n", description);
    });
    if (!glfwInit())
        exit(EXIT_FAILURE);
    //glfwWindowHint(GLFW_SCALE_TO_MONITOR, 1);
    if (es && !gfxthread) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    }
    if (gfxthread) { // we create gl context ourself, glfw should provide a clean window.
        // default is GLFW_OPENGL_API + GLFW_NATIVE_CONTEXT_API which may affect our context(macOS)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
    const int w = 640, h = 480;
    GLFWwindow *win = glfwCreateWindow(w, h, "MDK + GLFW. Drop videos here", nullptr, nullptr);
    if (!win) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetWindowUserPointer(win, &player);
    glfwSetKeyCallback(win, key_callback);
#if (GLFW_VERSION_MAJOR > 3 ||  GLFW_VERSION_MINOR > 2)
    if (glfwSetWindowContentScaleCallback)
        glfwSetWindowContentScaleCallback(win, [](GLFWwindow* win, float xscale, float yscale){
            printf("************window scale changed: %fx%f***********\n", xscale, yscale);
        });
#endif
    glfwSetFramebufferSizeCallback(win, [](GLFWwindow* win, int w,int h){
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        p->setVideoSurfaceSize(w, h);
    });
    glfwSetScrollCallback(win, [](GLFWwindow* win, double dx, double dy){
        //std::printf("scroll: %.03f, %.04f\n", dx, dy);fflush(stdout);
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        static float s = 1.0f;
        s += (float)dy/5.0f;
        p->scale(s, s);
    });
    glfwSetDropCallback(win, [](GLFWwindow* win, int count, const char** files){
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        p->setNextMedia(nullptr);
        p->setState(State::Stopped);
        urls.clear();
        for (int i = 0; i < count; ++i)
            urls.emplace_back(files[i]);
        url_now = 0;
        p->waitFor(State::Stopped);
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
    player.setPreloadImmediately(true); // MUST set before setMedia() because setNextMedia() is called when media is changed
    float xscale = 1.0f, yscale = 1.0f;
#if (GLFW_VERSION_MAJOR > 3 ||  GLFW_VERSION_MINOR > 2)
    if (glfwGetWindowContentScale)
        glfwGetWindowContentScale(win, &xscale, &yscale);
#endif
    int fw = 0, fh = 0;
    glfwGetFramebufferSize(win, &fw, &fh);
    printf("************fb size %dx%d, requested size: %dx%d, scale= %fx%f***********\n", fw, fh, w, h, xscale, yscale);
    player.setVideoSurfaceSize(fw, fh);
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

    if (loop >= -1)
        player.setLoop(loop);
    if (loop_a >= 0 || loop_b != 0) {
        if (loop_a < 0)
            loop_a = 0;
        if (loop_b == 0)
            loop_b = -1;
        player.setRange(loop_a, loop_b);// TODO: works before setMedia(..., Audio)
    }

    if (player.url()) {
        player.prepare(from*int64_t(TimeScaleForInt), [&player](int64_t t, bool*) {
            std::clog << ">>>>>>>>>>>>>>>>>prepared @" << t << std::endl; // FIXME: t is wrong http://qthttp.apple.com.edgesuite.net/1010qwoeiuryfg/sl.m3u8
            //std::clog << ">>>>>>>>>>>>>>>>>>>MediaInfo.duration: " << player.mediaInfo().duration << "<<<<<<<<<<<<<<<<<<<<" << std::endl;
#if MDK_VERSION_CHECK(0, 5, 0)
            return true;
#endif
        });
        if (!pause)
            player.setState(State::Playing);
    }

    if (gfxthread) {
        auto surface_type = MDK_NS::Player::SurfaceType::Auto;
#if defined(_WIN32)
        auto hwnd = glfwGetWin32Window(win);
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
        auto hwnd = glfwGetCocoaWindow(win);
#elif defined(GLFW_EXPOSE_NATIVE_X11)
        auto hwnd = glfwGetX11Window(win);
        surface_type = MDK_NS::Player::SurfaceType::X11;
#endif
        player.updateNativeSurface((void*)hwnd, -1, -1, surface_type);
        //player.showSurface(); // let glfw process events. event handling in mdk is only implemented in win32 and x11 for now
        //exit(EXIT_SUCCESS);
    }
    while (!glfwWindowShouldClose(win)) {
        if (!gfxthread) {
            glfwMakeContextCurrent(win);
            player.renderVideo();
            glfwSwapBuffers(win); // FIXME: old render buffer is displayed if render again after stopping by user. glfw bug?

        }
        //glfwWaitEventsTimeout(0.04); // if player.setRenderCallback() is not called, render at 25fps
        if (wait > 0)
            glfwWaitEventsTimeout(wait);
        else
            glfwWaitEvents();
    }
}
    //player.setVideoSurfaceSize(-1, -1); // it's better to cleanup gl renderer resources
    setLogHandler(nullptr);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
