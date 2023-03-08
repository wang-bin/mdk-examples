/*
 * Copyright (c) 2018-2023 WangBin <wbsecg1 at gmail.com>
 * MDK SDK example of multiple players
 */

#ifdef _WIN32
#include <d3d11.h>
#endif
#if __has_include(<vulkan/vulkan_core.h>) // FIXME: -I
# include <vulkan/vulkan_core.h>
#endif
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include "mdk/RenderAPI.h"
#include "mdk/Player.h"
#include "prettylog.h"

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
using namespace std;
using namespace std::chrono;
using namespace MDK_NS;

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

int main(int argc, const char*  argv[])
{
    printf("MDK + GLFW players\n");
    if (argc < 2) {
        printf("usage: %s [-es] [-share] [-fps int_fps] [-win win_count] [-c:v decoder] [-urls] video_urls\n"
            "-es: use OpenGL ES2+ instead of OpenGL\n"
            "-share: shared OpenGL/ES contexts\n"
            "-c:v: video decoder. can be FFmpeg, VideoToolbox, D3D11, DXVA, NVDEC, CUDA, VDPAU, VAAPI, MMAL(raspberry pi), CedarX(sunxi)\n"
        , argv[0]);
        printf("-win: number of windows, applied if no -urls option, assume only 1 url\n"
            "-urls: the number of windows is url count\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-plugins") == 0) {
            SetGlobalOption("plugins", argv[++i]);
            break;
        }
    }
    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "GLFW Error: %s\n", description);
    });
    if (!glfwInit())
        exit(EXIT_FAILURE);
//{
    auto monitor = glfwGetPrimaryMonitor();
    auto mode = glfwGetVideoMode(monitor);
    printf("primary screen size: %dx%d\n", mode->width, mode->height);
    int nb_win = 4;
    float wait = 0;
    bool share = false;
    bool es = false;
    const char* dec = nullptr;
    const char* const* urls = nullptr;
    int urls_idx = 0;
    float sync = 1.0f;
    const char* ao = nullptr;

#ifdef _WIN32
    D3D11RenderAPI d3d11ra{};
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
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-win") == 0) {
            nb_win = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-c:v") == 0) {
            dec = argv[++i];
        } else if (strcmp(argv[i], "-fps") == 0) {
            wait = 1.0f/atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-ao") == 0) {
            ao = argv[++i];
        } else if (strcmp(argv[i], "-share") == 0) {
            share = true;
        } else if (strcmp(argv[i], "-sync") == 0) {
            sync = std::stof(argv[++i]);
        } else if (strcmp(argv[i], "-es") == 0) {
            es = true;
        } else if (strcmp(argv[i], "-urls") == 0) {
            urls_idx = ++i;
        } else if (strstr(argv[i], "-d3d11") == argv[i]) {
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
                    d3d11ra.feature_level = (float)std::atof(value);
            });
# endif
#endif
        } else if (strstr(argv[i], "-gl") == argv[i]) {
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
                    glra.version = (float)std::atof(value);
            });
#endif // MDK_VERSION_CHECK(0, 8, 1) || defined(MDK_ABI)

        } else if (strstr(argv[i], "-metal") == argv[i]) {
#if (__APPLE__+0) && (MDK_VERSION_CHECK(0, 8, 2) || defined(MDK_ABI))
            ra = &mtlra;
            parse_options(argv[i] + sizeof("-metal") - 1, [&mtlra](const char* name, const char* value){
                if (strcmp(name, "device_index") == 0)
                    mtlra.device_index = std::atoi(value);
            });
#endif
        } else if (strstr(argv[i], "-vk") == argv[i]) {
#if (VK_VERSION_1_0+0) && (MDK_VERSION_CHECK(0, 10, 0) || defined(MDK_ABI))
            ra = &vkra;
            parse_options(argv[i] + sizeof("-vk") - 1, [&vkra](const char* name, const char* value){
                if (strcmp(name, "version") == 0)
                    vkra.max_version = VK_MAKE_VERSION((int)std::atof(value), int(std::atof(value)*10)%10, 0);
                else if (strcmp(name, "debug") == 0)
                    vkra.debug = std::atoi(value);
                else if (strcmp(name, "buffers") == 0)
                    vkra.buffers = std::atoi(value);
                else if (strcmp(name, "device_index") == 0)
                    vkra.device_index = std::atoi(value);
                else if (strcmp(name, "gfx_family") == 0)
                    vkra.graphics_family = std::atoi(value);
                else if (strcmp(name, "present_family") == 0)
                    vkra.present_family = std::atoi(value);
                else if (strcmp(name, "compute_family") == 0)
                    vkra.compute_family = std::atoi(value);
                else if (strcmp(name, "transfer_family") == 0)
                    vkra.transfer_family = std::atoi(value);
                else if (strcmp(name, "gfx_queue") == 0)
                    vkra.gfx_queue_index = std::atoi(value);
                printf("vkra.graphics_family: %d, vkra.max_version: %u\n", vkra.graphics_family, vkra.max_version);
            });
#endif
        }
    }
    if (urls_idx == 0)
        urls_idx = argc-1;
    urls = &argv[urls_idx];
    const int nb_urls = argc - urls_idx;
    if (nb_urls > 1)
        nb_win = nb_urls;
    const int dim = (int)ceil(sqrt(nb_win));
    const int w =  mode->width/dim, h =  mode->height/dim - 40;
    std::vector<GLFWwindow*> win(nb_win);
    std::vector<std::unique_ptr<Player>> players(nb_win);
    auto now = steady_clock::now();
    for (int i = 0; i < nb_win; i++) {
        auto& player = players[i];
        player.reset(new Player());
        //player->setPlaybackRate(2.0f);
        if (dec)
            player->setDecoders(MediaType::Video, {dec});
        //player->setActiveTracks(MediaType::Audio, {});
        if (sync != 1.0f)
            player->onSync([&]{
                return duration_cast<milliseconds>(steady_clock::now() - now).count() * sync / 1000.0;
            });
        if (ra)
            player->setRenderAPI(ra);
        if (!ra && wait <= 0)
            player->setRenderCallback([](void*){
                glfwPostEmptyEvent();
            });
        player->setMedia(urls[nb_urls > 1 ? i : 0]);
        if (ao)
            player->setAudioBackends({ao});
        player->setLoop(-1);
        player->prepare();
#if 0
// test different context profiles and versions
        if (i >= 2 && !share) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        }
#endif
        if (ra) {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        } else if (es) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        }
        player->setVideoSurfaceSize(w, h);
        win[i] = glfwCreateWindow(w, h, ("MDK + GLFW multi-window" + to_string(i)).data(), nullptr, share ? win[0] : nullptr);
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

        if (ra) {
            auto surface_type = MDK_NS::Player::SurfaceType::Auto;
#if defined(_WIN32)
            auto hwnd = glfwGetWin32Window(win[0]);
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
            auto hwnd = glfwGetCocoaWindow(win[0]);
#elif defined(GLFW_EXPOSE_NATIVE_X11)
            auto hwnd = glfwGetX11Window(win[0]);
            surface_type = MDK_NS::Player::SurfaceType::X11;
#endif
            player->updateNativeSurface((void*)hwnd, -1, -1, surface_type);
            //player.showSurface(); // let glfw process events. event handling in mdk is only implemented in win32 and x11 for now
            //exit(EXIT_SUCCESS);
        }
    }

    for (auto& player : players)
        player->set(State::Playing);
    while (true) {
        for (int i = 0; i < nb_win; ++i) {
            auto& w = win[i];
            if (!w)
                continue;
            auto& player = players[i];
            if (!player)
                continue;
            if (!ra) {
                glfwMakeContextCurrent(w);
                player->renderVideo();
                glfwSwapBuffers(w);
            }

            if (glfwWindowShouldClose(w)) {
                glfwMakeContextCurrent(w);
                player->setVideoSurfaceSize(-1, -1);
                player.reset();

                const auto remaining = std::count_if(players.begin(), players.end(), [](const auto& p) { return p.get(); });
                if (!remaining)
                    goto end;
            }
        }
        if (wait > 0)
            glfwWaitEventsTimeout(wait);
        else
            glfwWaitEvents();
    }
loopend:
    for (int i = 0; i < nb_win; ++i) {
        auto& player = players[i];
        auto& w = win[i];
        // cleanup renderer and gl resources in render context(current)
        glfwMakeContextCurrent(w);
        player->setVideoSurfaceSize(-1, -1);
    }
//}
end:
    glfwTerminate();
    exit(EXIT_SUCCESS);
}