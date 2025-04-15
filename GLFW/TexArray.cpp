
/*
 * Copyright (c) 2016-2025 WangBin <wbsecg1 at gmail.com>
 * MDK SDK + GLFW example
 */
#pragma optimize( "", off )

#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif
#ifdef _WIN32
#include <d3d11.h>
#include <d3d12.h>

// vista is required to build with wrl, but xp runtime works
#pragma push_macro("_WIN32_WINNT")
#if _WIN32_WINNT < _WIN32_WINNT_VISTA
# undef _WIN32_WINNT
# define _WIN32_WINNT _WIN32_WINNT_VISTA
#endif
#include <wrl/client.h>
#if (_MSC_VER + 0) // missing headers in mingw
#include <wrl/implements.h> // RuntimeClass
#endif
#pragma pop_macro("_WIN32_WINNT")
using namespace Microsoft::WRL; //ComPtr

#endif
#if __has_include(<vulkan/vulkan_core.h>) // FIXME: -I
# include <vulkan/vulkan_core.h>
#endif
#include "mdk/MediaInfo.h"
#include "mdk/RenderAPI.h"
#include "mdk/Player.h"
#include "mdk/AudioFrame.h"
#include "mdk/VideoFrame.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <regex>
#include <string>
#include <vector>
#ifdef _WIN32
#include <filesystem>
#endif
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

using namespace MDK_NS;

void* vid = nullptr;

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
int main(int argc, const char** argv)
{
    //setlocale(LC_ALL, "de_CH");

    FILE* log_file = stdout;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-logfile") == 0) {
            log_file = fopen(argv[++i], "w");
            break;
        }
    }
    setLogHandler([&log_file](LogLevel level, const char* msg){
        static const char level_name[] = {'N', 'E', 'W', 'I', 'D', 'A'};
        print_log_msg(level_name[level], msg, log_file);
    });
    //setLogLevel(LogLevel::Error);
{
    bool help = argc < 2;
    bool gpa_glfw = false;
    bool es = false;
    float speed = 1.0f;
    double from = 0;
    bool pause = false;
    int arraySize = 1;
    int arraySlice = 0;
    string vendor;
    std::string ca, cv;
    int w = 640;
    int h = 480;
    // some plugins must be loaded before creating player
    for (int i = 1; i < argc; ++i) { // "plugins_dir" and "plugins" MUST be set before player constructed
        if (std::strncmp(argv[i], "-plugins", strlen("-plugins")) == 0) {
            SetGlobalOption(argv[i] + 1, (const char*)argv[i+1]);
            i++;
            break;
        }
    }

    Player rttPlayer;
    Player player;
    player.set(ColorSpaceUnknown);
#ifdef _WIN32
    ComPtr<ID3D11Device> dev11;
    ComPtr<ID3D11Texture2D> tex11;
    D3D11RenderAPI d3d11ra{};
    D3D12RenderAPI d3d12ra{};
#endif
#if (__APPLE__+0) && (MDK_VERSION_CHECK(0, 8, 2) || defined(MDK_ABI))
    MetalRenderAPI mtlra;
#endif
#if MDK_VERSION_CHECK(0, 8, 2) || defined(MDK_ABI)
    GLRenderAPI glra{};
#endif
#if (VK_VERSION_1_0+0) && (MDK_VERSION_CHECK(0, 10, 0) || defined(MDK_ABI))
    VulkanRenderAPI vkra{};
#endif
    RenderAPI *ra = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-bg") == 0) {
            auto c = std::stoll(argv[++i], nullptr, 16);
            const auto r = c >> 24;
            const auto g = (c >> 16) & 0xff;
            const auto b = (c >> 8) & 0xff;
            const auto a = c & 0xff;
            player.setBackgroundColor(float(r)/255.0, float(g)/255.0, float(b)/255.0, float(a)/255.0);
        } else if (strcmp(argv[i], "-size") == 0) {
            char *s = (char *)argv[++i];
            w = strtoll(s, &s, 10);
            if (s[0])
                h = strtoll(&s[1], nullptr, 10);
        } else if (std::strcmp(argv[i], "-speed") == 0) {
            speed = (float)atof(argv[++i]);
        } else if (strcmp(argv[i], "-colorspace") == 0) {
            auto cs = argv[++i];
            if (strcmp(cs, "auto") == 0) {
                player.set(ColorSpaceUnknown);
            } else if (strcmp(cs, "bt709") == 0) {
                player.set(ColorSpaceBT709);
            } else if (strcmp(cs, "bt2100") == 0) {
                player.set(ColorSpaceBT2100_PQ);
            } else if (strcmp(cs, "scrgb") == 0) {
                player.set(ColorSpaceSCRGB);
            } else if (strcmp(cs, "srgbl") == 0) {
                player.set(ColorSpaceExtendedLinearSRGB);
            } else if (strcmp(cs, "srgbf") == 0) {
                player.set(ColorSpaceExtendedSRGB);
            } else if (strcmp(cs, "p3") == 0) {
                player.set(ColorSpaceExtendedLinearDisplayP3);
            }
        } else if (strstr(argv[i], "-d3d11") == argv[i] && (argv[i][6] == 0 || argv[i][6] == ':')) {
#ifdef _WIN32
            ra = &d3d11ra;
# if MDK_VERSION_CHECK(0, 8, 1) || defined(MDK_ABI)
            parse_options(argv[i] + sizeof("-d3d11") - 1, [&](const char* name, const char* value){
                if (strcmp(name, "debug") == 0)
                    d3d11ra.debug = std::atoi(value);
                else if (strcmp(name, "buffers") == 0)
                    d3d11ra.buffers = std::atoi(value);
                else if (strcmp(name, "adapter") == 0)
                    d3d11ra.adapter = std::atoi(value);
                else if (strcmp(name, "feature_level") == 0)
                    d3d11ra.feature_level = (float)std::atof(value);
                else if (strcmp(name, "vendor") == 0) {
                    vendor = value;
                    d3d11ra.vendor = vendor.data();
                    std::printf("argv vender: %s\n", vendor.data());fflush(0);
                }
            });
# endif
#endif
        } else if (strstr(argv[i], "-metal") == argv[i] && (argv[i][6] == 0 || argv[i][6] == ':')) {
#if (__APPLE__+0) && (MDK_VERSION_CHECK(0, 8, 2) || defined(MDK_ABI))
            ra = &mtlra;
            parse_options(argv[i] + sizeof("-metal") - 1, [&mtlra](const char* name, const char* value){
                if (strcmp(name, "device_index") == 0)
                    mtlra.device_index = std::atoi(value);
            });
#endif
        } else if (strcmp(argv[i], "-gpa_glfw") == 0) {
            gpa_glfw = true;
        } else if (strcmp(argv[i], "-es") == 0) {
            es = true;
        } else if (std::strcmp(argv[i], "-array") == 0) {
            arraySize = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-from") == 0) {
            from = std::atof(argv[++i]);
        } else if (std::strcmp(argv[i], "-pause") == 0) {
            pause = true;
        } else if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "-help") == 0) {
            help = true;
            break;
        } else if (argv[i][0] == '-' && (argv[i+1][0] == '-' || i == argc - 2)) {
            printf("Unknow option: %s\n", argv[i]);
        } else if (argv[i][0] == '-') {
            printf("SetGlobalOption: %s=%s\n", argv[i]+1, argv[i+1]);
            SetGlobalOption(argv[i]+1, argv[i+1]);
            player.setProperty(argv[i]+1, argv[i+1]);
            ++i;
        } else {
            for (int j = i; j < argc; ++j) {
                urls.emplace_back(argv[j]);
            }
            break;
        }
    }

#ifdef  _WIN32
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &dev11, nullptr, nullptr);
    ComPtr<ID3D10Multithread> mt;
    if (SUCCEEDED(dev11.As(&mt)))
        mt->SetMultithreadProtected(TRUE);
    const D3D11_TEXTURE2D_DESC desc = {
        .Width = 1280,
        .Height = 720,
        .MipLevels = 1,
        .ArraySize = (UINT)arraySize,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0,
        },
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
        .CPUAccessFlags = 0,
        .MiscFlags = D3D11_RESOURCE_MISC_SHARED,
    };
    dev11->CreateTexture2D(&desc, nullptr, &tex11);
    D3D11RenderAPI rttRa{};
    rttRa.rtv = tex11.Get();
    rttPlayer.setRenderAPI(&rttRa);
    arraySlice = 0;
    rttPlayer.setVideoSurfaceSize(1280, 720);
#endif

    if (!glfwInit())
        exit(EXIT_FAILURE);
    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "Error: %s\n", description);
    });
    if (ra) { // we create gl context ourself, glfw should provide a clean window.
        // default is GLFW_OPENGL_API + GLFW_NATIVE_CONTEXT_API which may affect our context(macOS)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
    GLFWwindow *win = glfwCreateWindow(w, h, "MDK + GLFW. Drop videos here", nullptr, nullptr);
    if (!win) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetWindowUserPointer(win, &player);
    glfwSetFramebufferSizeCallback(win, [](GLFWwindow* win, int w,int h){
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        float xscale = 1.0f, yscale = 1.0f;
#if (GLFW_VERSION_MAJOR > 3 ||  GLFW_VERSION_MINOR > 2)
    if (glfwGetWindowContentScale)
        glfwGetWindowContentScale(win, &xscale, &yscale);
#endif
        printf("************framebuffer size changed: %dx%d, scale: %f***********\n", w, h, xscale);
        if (vid)
            p->updateNativeSurface(vid, w, h); // for x11. win32/cocoa can detect size change
        else
            p->setVideoSurfaceSize(w, h);
    });

    glfwShowWindow(win);
    rttPlayer.onStateChanged([=](State s){
        if (s == State::Stopped) {
            glfwSetWindowShouldClose(win, 1);
            glfwPostEmptyEvent();
        }
    });
    float xscale = 1.0f, yscale = 1.0f;
#if (GLFW_VERSION_MAJOR > 3 ||  GLFW_VERSION_MINOR > 2)
    if (glfwGetWindowContentScale)
        glfwGetWindowContentScale(win, &xscale, &yscale);
#endif
// setDecoders before setMedia(), setNextMedia() in currentMediaChanged() callback may load immediately

    int fw = 0, fh = 0;
    glfwGetFramebufferSize(win, &fw, &fh);
    printf("************fb size %dx%d, requested size: %dx%d, scale= %fx%f***********\n", fw, fh, w, h, xscale, yscale);
    //player.setPlaybackRate(2.0f);
    if (!urls.empty()) {
#ifdef _WIN32
        filesystem::path p(urls[url_now].data());
        rttPlayer.setMedia((char*)p.u8string().data());
#else
        rttPlayer.setMedia(urls[url_now].data());
#endif
    }
    if (!cv.empty()) {
        std::regex re(",");
        std::sregex_token_iterator first{cv.begin(), cv.end(), re, -1}, last;
        rttPlayer.setDecoders(MediaType::Video, {first, last});
    }
    if (rttPlayer.url()) {
        rttPlayer.prepare(int64_t(from*TimeScaleForInt), [&](int64_t t, bool*) {
            std::clog << ">>>>>>>>>>>>>>>>>prepared @" << t << std::endl; // FIXME: t is wrong http://qthttp.apple.com.edgesuite.net/1010qwoeiuryfg/sl.m3u8
            //std::clog << ">>>>>>>>>>>>>>>>>>>MediaInfo.duration: " << player.mediaInfo().duration << "<<<<<<<<<<<<<<<<<<<<" << std::endl;
            return true;
        });
        if (!pause)
            rttPlayer.set(State::Playing);
    }

    if (ra) {
        auto surface_type = MDK_NS::Player::SurfaceType::Auto;
#if defined(_WIN32)
        auto hwnd = glfwGetWin32Window(win);
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
        auto hwnd = glfwGetCocoaWindow(win);
#endif
        vid = (void*)hwnd;
        player.setRenderAPI(ra, vid);
        player.updateNativeSurface(vid, fw /*-1*/, fh /*-1*/, surface_type);
        //player.showSurface(); // let glfw process events. event handling in mdk is only implemented in win32 and x11 for now
        //exit(EXIT_SUCCESS);
    }

    rttPlayer.setPlaybackRate(speed);
    double lastTime = -1;
    NativeVideoBufferPool::Ptr pool;
    rttPlayer.setRenderCallback([&](void* opaque) {
        auto t = rttPlayer.renderVideo(opaque);
        t = rttPlayer.renderVideo(opaque);
        const DX11Resource res{
            .resource = tex11.Get(),
            .subResource = arraySlice,
        };
        auto f = VideoFrame::from(&pool, res);
        player.enqueue(f, vid);
        if (t != lastTime) {
            arraySlice = (arraySlice + 1) % arraySize;
            lastTime = t;
        }
    });
    while (!glfwWindowShouldClose(win)) {
        //glfwWaitEventsTimeout(0.04); // if player.setRenderCallback() is not called, render at 25fps
        glfwWaitEvents();
    }
}
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
