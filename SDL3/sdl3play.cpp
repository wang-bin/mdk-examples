/*
 * Copyright (c) 2016-2026 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with SDL3 example
 */
#include <mdk/Player.h>
#include <mdk/RenderAPI.h>
#ifdef _WIN32
#include <d3d11.h>
#include <d3d12.h>
#endif
#if __has_include(<vulkan/vulkan_core.h>)
# include <vulkan/vulkan_core.h>
#endif
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

using namespace MDK_NS;

int print_help(const char* app) {
    return printf("usage: %s [-from seconds] [-ao audio_renderer] [-c:v video_decoder] [-d3d11|-d3d12|-metal|-vk] url1 [url2 ...]\n", app);
}

template<typename Func>
void parse_options(const char* opts, Func cb)
{
    if (!opts || !opts[0])
        return;
    while (opts[0] == ':')
        opts++;
    auto options = std::string(opts);
    char* p = &options[0];
    while (true) {
        char* v = std::strchr(p, '=');
        if (!v)
            break;
        *v++ = 0;
        char* pp = std::strchr(v, ':');
        if (pp)
            *pp = 0;
        cb(p, v);
        if (!pp)
            break;
        p = pp + 1;
    }
}

struct NativeSurfaceInfo {
    void* handle = nullptr;
    Player::SurfaceType type = Player::SurfaceType::Auto;
};

NativeSurfaceInfo nativeSurface(SDL_Window* window)
{
    NativeSurfaceInfo ns;
    const auto props = SDL_GetWindowProperties(window);
#if defined(SDL_PLATFORM_WIN32)
    ns.handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(SDL_PLATFORM_MACOS)
    ns.handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#elif defined(SDL_PLATFORM_LINUX)
    const auto* driver = SDL_GetCurrentVideoDriver();
    if (driver && std::strcmp(driver, "x11") == 0) {
        const auto xwin = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        ns.handle = reinterpret_cast<void*>(static_cast<uintptr_t>(xwin));
        ns.type = Player::SurfaceType::X11;
    } else if (driver && std::strcmp(driver, "wayland") == 0) {
        ns.handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
        ns.type = Player::SurfaceType::Wayland;
    }
#endif
    return ns;
}

int main(int argc, const char *argv[])
{
    if (argc < 2)
        return print_help(argv[0]);
    int url_index = 1;
    int from = 0;
    const char* cv = nullptr;
    const char* ao = nullptr;
    bool help = false;
    bool use_vk = false;
    bool use_metal = false;
    std::string vendor;
#ifdef _WIN32
    D3D11RenderAPI d3d11ra{};
    D3D12RenderAPI d3d12ra{};
#endif
#if (__APPLE__+0) && (MDK_VERSION_CHECK(0, 8, 2) || defined(MDK_ABI))
    MetalRenderAPI mtlra{};
#endif
#if (VK_VERSION_1_0+0) && (MDK_VERSION_CHECK(0, 10, 0) || defined(MDK_ABI))
    VulkanRenderAPI vkra{};
#endif
    RenderAPI* ra = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-c:v") == 0 && i + 1 < argc) {
            cv = argv[++i];
            url_index += 2;
        } else if (std::strcmp(argv[i], "-from") == 0 && i + 1 < argc) {
            from = std::atoi(argv[++i]);
            url_index += 2;
        } else if (std::strcmp(argv[i], "-ao") == 0 && i + 1 < argc) {
            ao = argv[++i];
            url_index += 2;
        } else if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "-help") == 0) {
            help = true;
            break;
        } else if (std::strstr(argv[i], "-d3d11") == argv[i] && (argv[i][6] == 0 || argv[i][6] == ':')) {
#ifdef _WIN32
            ra = &d3d11ra;
# if MDK_VERSION_CHECK(0, 8, 1) || defined(MDK_ABI)
            parse_options(argv[i] + sizeof("-d3d11") - 1, [&](const char* name, const char* value) {
                if (std::strcmp(name, "debug") == 0)
                    d3d11ra.debug = std::atoi(value);
                else if (std::strcmp(name, "buffers") == 0)
                    d3d11ra.buffers = std::atoi(value);
                else if (std::strcmp(name, "adapter") == 0)
                    d3d11ra.adapter = std::atoi(value);
                else if (std::strcmp(name, "feature_level") == 0)
                    d3d11ra.feature_level = static_cast<float>(std::atof(value));
                else if (std::strcmp(name, "vendor") == 0) {
                    vendor = value;
                    d3d11ra.vendor = vendor.data();
                }
            });
# endif
#endif
            ++url_index;
        } else if (std::strstr(argv[i], "-d3d12") == argv[i] && (argv[i][6] == 0 || argv[i][6] == ':')) {
#ifdef _WIN32
            ra = &d3d12ra;
            parse_options(argv[i] + sizeof("-d3d12") - 1, [&](const char* name, const char* value) {
                if (std::strcmp(name, "debug") == 0)
                    d3d12ra.debug = std::atoi(value);
                else if (std::strcmp(name, "buffers") == 0)
                    d3d12ra.buffers = std::atoi(value);
                else if (std::strcmp(name, "adapter") == 0)
                    d3d12ra.adapter = std::atoi(value);
                else if (std::strcmp(name, "feature_level") == 0)
                    d3d12ra.feature_level = static_cast<float>(std::atof(value));
                else if (std::strcmp(name, "vendor") == 0) {
                    vendor = value;
                    d3d12ra.vendor = vendor.data();
                }
            });
#endif
            ++url_index;
        } else if (std::strstr(argv[i], "-metal") == argv[i] && (argv[i][6] == 0 || argv[i][6] == ':')) {
#if (__APPLE__+0) && (MDK_VERSION_CHECK(0, 8, 2) || defined(MDK_ABI))
            ra = &mtlra;
            use_metal = true;
            parse_options(argv[i] + sizeof("-metal") - 1, [&](const char* name, const char* value) {
                if (std::strcmp(name, "device_index") == 0)
                    mtlra.device_index = std::atoi(value);
            });
#endif
            ++url_index;
        } else if (std::strstr(argv[i], "-vk") == argv[i] && (argv[i][3] == 0 || argv[i][3] == ':')) {
#if (VK_VERSION_1_0+0) && (MDK_VERSION_CHECK(0, 10, 0) || defined(MDK_ABI))
            ra = &vkra;
            use_vk = true;
            parse_options(argv[i] + sizeof("-vk") - 1, [&](const char* name, const char* value) {
                if (std::strcmp(name, "version") == 0)
                    vkra.max_version = VK_MAKE_VERSION((int)std::atof(value), int(std::atof(value) * 10) % 10, 0);
                else if (std::strcmp(name, "debug") == 0)
                    vkra.debug = std::atoi(value);
                else if (std::strcmp(name, "buffers") == 0)
                    vkra.buffers = std::atoi(value);
                else if (std::strcmp(name, "device_index") == 0)
                    vkra.device_index = std::atoi(value);
            });
#endif
            ++url_index;
        } else if (argv[i][0] == '-' && i + 1 < argc) {
            SetGlobalOption(argv[i] + 1, argv[i + 1]);
            i++;
            url_index += 2;
        }
    }

    if (help || url_index >= argc)
        return print_help(argv[0]);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    Uint64 window_flags = SDL_WINDOW_RESIZABLE;
    if (!ra)
        window_flags |= SDL_WINDOW_OPENGL;
    if (use_vk)
        window_flags |= SDL_WINDOW_VULKAN;
    if (use_metal)
        window_flags |= SDL_WINDOW_METAL;
    SDL_Window* window = SDL_CreateWindow("MDK player with SDL3 Rendering", 800, 500, window_flags);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    SDL_GLContext glctx = nullptr;
    if (!ra) {
        glctx = SDL_GL_CreateContext(window);
        if (!glctx) {
            std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
            SDL_DestroyWindow(window);
            SDL_Quit();
            return -1;
        }
    }
    SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, true);
    const Uint32 update_event = SDL_RegisterEvents(1);

    int w = 800, h = 500;
    SDL_GetWindowSizeInPixels(window, &w, &h);

    int url_now = url_index;
    Player player;
    if (cv)
        player.setDecoders(MediaType::Video, {cv});
    if (ao)
        player.setAudioBackends({ao});
    player.currentMediaChanged([&] {
        std::printf("currentMediaChanged %d/%d, now: %s\n", url_now - url_index, argc - url_index, player.url());
        std::fflush(stdout);
        if (argc > url_now + 1) {
            ++url_now;
            player.setNextMedia(argv[url_now]);
        }
    });
    player.onMediaStatus([](MediaStatus s) {
        std::printf("************Media status: %#x, loading: %d, buffering: %d, prepared: %d, EOF: %d**********\n",
                    s, s & MediaStatus::Loading, s & MediaStatus::Buffering, s & MediaStatus::Prepared, s & MediaStatus::End);
        return true;
    });
    player.onEvent([](const MediaEvent& e) {
        std::cout << "event: " << e.category << ", detail: " << e.detail << std::endl;
        return false;
    });
    if (!ra) {
        player.setRenderCallback([=](void*) {
            SDL_Event e{};
            e.type = update_event;
            SDL_PushEvent(&e);
        });
        player.setVideoSurfaceSize(w, h);
    } else {
        const auto ns = nativeSurface(window);
        if (ns.handle) {
            player.setRenderAPI(ra, ns.handle);
            player.updateNativeSurface(ns.handle, w, h, ns.type);
        } else {
            std::fprintf(stderr, "No SDL native surface handle available for selected backend\n");
        }
    }

    player.setMedia(argv[url_index]);
    player.prepare(from * int64_t(TimeScaleForInt), [](int64_t t, bool*) {
        std::cout << ">>>>>>>>>>>>>>>>>prepared @" << t << std::endl;
        return true;
    });
    player.set(State::Playing);

    bool running = true;
    while (running) {
        SDL_Event event;
        if (!SDL_WaitEvent(&event))
            continue;
        switch (event.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            running = false;
            break;
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            SDL_GetWindowSizeInPixels(window, &w, &h);
            if (ra) {
                const auto ns = nativeSurface(window);
                if (ns.handle)
                    player.updateNativeSurface(ns.handle, w, h, ns.type);
            } else {
                player.setVideoSurfaceSize(w, h);
            }
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_SPACE) {
                player.set(player.state() == State::Playing ? State::Paused : State::Playing);
            } else if (event.key.key == SDLK_A) {
                static const float ap[] = {IgnoreAspectRatio, KeepAspectRatio, KeepAspectRatioCrop};
                static size_t ai = 0;
                player.setAspectRatio(ap[(ai++) % 3]);
            } else if (event.key.key == SDLK_O) {
                static int angle = 0;
                player.rotate(angle += 90);
            } else if (event.key.key == SDLK_S) {
                player.set(State::Stopped);
            } else if (event.key.key == SDLK_P) {
                player.set(State::Playing);
            } else if (event.key.key == SDLK_Q) {
                running = false;
            } else if (event.key.key == SDLK_RIGHT) {
                std::printf("seekForward from: %" PRId64 "ms\n", player.position());
                player.seek(player.position() + 10 * int(TimeScaleForInt), [](int64_t t) {
                    std::printf("seek finished at: %" PRId64 "ms\n", t);
                });
            } else if (event.key.key == SDLK_LEFT) {
                std::printf("seekBackward from: %" PRId64 "ms\n", player.position());
                player.seek(player.position() - 10 * int(TimeScaleForInt), [](int64_t t) {
                    std::printf("seek finished at: %" PRId64 "ms\n", t);
                });
            } else if (event.key.key == SDLK_UP) {
                const auto v = player.playbackRate();
                player.setPlaybackRate(v + 0.2f);
            } else if (event.key.key == SDLK_DOWN) {
                const auto v = player.playbackRate();
                player.setPlaybackRate(v - 0.2f);
            } else if (event.key.key == SDLK_N) {
                static int index = 1;
                const int nb_urls = argc - url_index;
                if (nb_urls > 1) {
                    const auto index0 = url_index;
                    url_index = (index++) % nb_urls + index0;
                    url_now = url_index;
                    player.setMedia(argv[url_index]);
                    player.set(State::Playing);
                }
            }
            break;
        case SDL_EVENT_DROP_FILE:
            player.setNextMedia(nullptr);
            player.set(State::Stopped);
            player.waitFor(State::Stopped);
            player.setMedia(nullptr);
            player.setMedia(event.drop.data);
            player.set(State::Playing);
            break;
        default:
            if (!ra && event.type == update_event) {
                player.renderVideo();
                SDL_GL_SwapWindow(window);
            }
            break;
        }
    }

    player.set(State::Stopped);
    if (!ra)
        player.setVideoSurfaceSize(-1, -1);
    if (glctx)
        SDL_GL_DestroyContext(glctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
