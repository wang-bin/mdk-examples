#define _CRT_SECURE_NO_WARNINGS
#include "mdk/Player.h"
#include <cstring>
#include <cstdio>
#include <regex>
#include <string_view>
#include <unordered_map>
#include <iostream>
#if 0//def _WIN32
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

using namespace MDK_NS;
using namespace std;

int print_help(const char* app) {
    return printf("usage: %s [-size widthxheight] [-ao audio_renderer] [-c:v video_decoder] url [url1 url2 ...]\n", app);
}

static Player::SurfaceType surface_type(const char* name)
{
    if (!name)
        return Player::SurfaceType::Auto;
    static const unordered_map<string_view, Player::SurfaceType> types {
        {"auto", Player::SurfaceType::Auto },
        {"x11", Player::SurfaceType::X11 },
        {"wayland", Player::SurfaceType::Wayland },
        {"gbm", Player::SurfaceType::GBM },
    };
    auto it = types.find(name);
    if (it == types.cend())
        return Player::SurfaceType::Auto;
    return it->second;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        return print_help(argv[0]);
    int url_index = 1;
    int from = 0;
    int w = 1920, h=1080;
    std::string cv;
    const char* ao = nullptr;
    const char* surface = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-c:v") == 0) { // glx on mac works with vt 0-copy
            cv = argv[++i];
            url_index += 2;
        } else if(std::strcmp(argv[i], "-size") == 0) {
            std::sscanf(argv[++i], "%dx%d", &w, &h);
            url_index += 2;
        } else if (std::strcmp(argv[i], "-from") == 0) {
            from = std::atoi(argv[++i]);
            url_index += 2;
        } else if (std::strcmp(argv[i], "-ao") == 0) {
            ao = argv[++i];
            url_index += 2;
        } else if (std::strcmp(argv[i], "-surface") == 0) {
            surface = argv[++i];
            url_index += 2;
        } else if (argv[i][0] == '-') {
            return print_help(argv[0]);
        }
    }
    int url_now = url_index;
    Player p;
    if (ao)
        p.setAudioBackends({ao});
    if (!cv.empty()) {
        std::regex re(",");
        std::sregex_token_iterator first{cv.begin(), cv.end(), re, -1}, last;
        p.setVideoDecoders({first, last});
    }
    p.onEvent([](const MediaEvent& e){
        std::clog << "***************************event: " << e.category << ", detail: " <<e.detail << std::endl;
        return false;
    });

#if 0//def _WIN32
    if (!glfwInit())
        return -1;
    GLFWwindow* win = glfwCreateWindow(w, h, "MDK + GLFW", nullptr, nullptr);
    HWND hwnd = glfwGetWin32Window(win);
    p.updateNativeSurface(hwnd);
#else
    p.createSurface(nullptr, surface_type(surface)); // "x11", "wayland" ...s
    p.resizeSurface(w, h); 
#endif 
    p.currentMediaChanged([&]{
        printf("currentMediaChanged %d/%d, now: %s\n", url_now-url_index, argc-url_index, p.url());fflush(stdout);
        if (argc > url_now+1)
            p.setNextMedia(argv[++url_now]);
    });
    p.onStateChanged([](State s){
        if (s == State::Stopped)
            exit(0);
    });
    p.setMedia(argv[url_now]);
    //if (from > 0)
        p.prepare(from*int64_t(TimeScaleForInt), [&p](int64_t t, bool*) {
            std::cout << ">>>>>>>>>>>>>>>>>prepared @" << t << std::endl; // FIXME: t is wrong http://qthttp.apple.com.edgesuite.net/1010qwoeiuryfg/sl.m3u8
        });
    p.setState(State::Playing);
#if 0//def _WIN32
    while (true) {
        if (glfwWindowShouldClose(win))
            break;
        glfwWaitEvents();
    }
    glfwTerminate();
    return 0;
#endif
    p.showSurface(); // TODO: no internal event loop for a foreign hwnd
    return 0;
}
