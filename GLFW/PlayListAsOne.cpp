
/*
 * Copyright (c) 2020-2021 WangBin <wbsecg1 at gmail.com>
 * MDK SDK Playlist as 1 video
 */
#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif
#include "mdk/RenderAPI.h"
#include "mdk/Player.h"
#include "mdk/MediaInfo.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <regex>
#include <GLFW/glfw3.h>
#include "prettylog.h"

using namespace MDK_NS;

int64_t gSeekStep = 10000LL;
SeekFlag gSeekFlag = SeekFlag::Default;

struct PlaylistItem {
    string url;
    int64_t startTime = 0; // ms
    int64_t duration = 0; // ms
};


int item_now = 0;
std::vector<PlaylistItem> items;
int64_t gDuration = 0; //ms

static void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;
    auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
    switch (key) {
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
            p->setDecoders(MediaType::Video, {decs[++d%std::size(decs)]});
        }
            break;
        case GLFW_KEY_H: {
            static float h = 0;
            h += 0.1f;
            if (h > 1.0)
                h = -1.0f;
            p->set(VideoEffect::Hue, h);
        }
            break;
        case GLFW_KEY_SPACE:
            p->set(p->state() == State::Playing ? State::Paused : State::Playing);
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
            p->set(State::Stopped);
            break;
        default:
            break;
    }
}

int64_t ItemsFromUrls(std::vector<PlaylistItem>* items, const char** urls, int count)
{
    items->clear();
    vector<Player> infoReader(count);
    for (int i = 0; i < count; ++i) {
        PlaylistItem item;
        item.url = urls[i];
        items->push_back(move(item));
        auto reader = &infoReader[i];
        reader->setMedia(urls[i]);
        reader->prepare(); // callback [](int64_t, bool*){ return false;} will result in empty mediaInfo
    }
    int64_t duration = 0;
    for (int i = 0; i < count; ++i) {
        auto reader = &infoReader[i];
        reader->waitFor(State::Paused); // prepare() will change to State::Paused when completed
        duration += reader->mediaInfo().duration;
        items->at(i).duration = reader->mediaInfo().duration;
        if (i > 0)
            items->at(i).startTime = items->at(i-1).startTime + items->at(i-1).duration;
        printf("++++++++++%lld+%lld: %s\n", items->at(i).startTime, duration, items->at(i).url.data());
    }
    return duration;
}

void Seek(Player* player, const vector<PlaylistItem>& items, int64_t position)
{
    auto it = find_if(items.cbegin(), items.cend(), [position](const PlaylistItem& item){
        return item.duration > 0 && item.startTime <= position && (item.startTime + item.duration) > position;
    });
    if (it == items.cend())
        return;
    auto offset = position - it->startTime;
    if (it->url == player->url()) {
        player->seek(offset, SeekFlag::FromStart);
        return;
    }
    item_now = distance(it, items.cbegin());
    player->setNextMedia(nullptr);
    player->set(State::Stopped);
    player->waitFor(State::Stopped);
    player->setMedia(it->url.data());
    player->prepare(offset);
    player->set(State::Paused);
}

int main(int argc, const char** argv)
{
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
    SetGlobalOption("videoout.clear_on_stop", 0); // no clear video renderer if seek to another playlist item

{
    int from = 0;
    int loop = -2;
    int64_t loop_a = -1;
    int64_t loop_b = 0;
    std::string cv;
    Player player;

    player.setProperty("continue_at_end", "1"); // do not stop when the last frame is decoded
    //player.setBackgroundColor(1, 0, 0, 1);
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c:v") == 0) {
            cv = argv[++i];
        } else if (std::strcmp(argv[i], "-from") == 0) {
            from = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-seek_any") == 0) {
            gSeekFlag = SeekFlag::FromStart;
        } else if (std::strcmp(argv[i], "-seek_step") == 0) {
            gSeekStep = atoi(argv[++i]);
        } else if (argv[i][0] == '-' && (argv[i+1][0] == '-' || i == argc - 2)) {
            printf("Unknow option: %s\n", argv[i]);
        } else if (argv[i][0] == '-') {
            SetGlobalOption(argv[i]+1, argv[i+1]);
            player.setProperty(argv[i]+1, argv[i+1]);
            ++i;
        } else {
            printf("++++++++++++++++++++\n");
            gDuration = ItemsFromUrls(&items, &argv[i], argc - i);
            break;
        }
    }
    player.currentMediaChanged([&]{
        std::printf("currentMediaChanged %d/%zu, now: %s\n", item_now, items.size(), player.url());fflush(stdout);
        if (items.size() > item_now+1) {
            player.setNextMedia(items[++item_now].url.data()); // safe to use ref to player
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

    player.setRenderCallback([](void*){
        glfwPostEmptyEvent(); // FIXME: some events are lost on macOS. glfw bug?
    });

    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "Error: %s\n", description);
    });
    if (!glfwInit())
        exit(EXIT_FAILURE);
    const int w = 640, h = 480;
    GLFWwindow *win = glfwCreateWindow(w, h, "Drop videos here. press Z & move mouse to seek", nullptr, nullptr);
    if (!win) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetWindowUserPointer(win, &player);
    glfwSetKeyCallback(win, key_callback);
    glfwSetFramebufferSizeCallback(win, [](GLFWwindow* win, int w,int h){
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        printf("************framebuffer size changed: %dx%d***********\n", w, h);
        p->setVideoSurfaceSize(w, h);
    });
    glfwSetDropCallback(win, [](GLFWwindow* win, int count, const char** files){
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        p->setNextMedia(nullptr);
        p->set(State::Stopped);
        gDuration = ItemsFromUrls(&items, files, count);
        item_now = 0;
        p->waitFor(State::Stopped);
        p->setMedia(nullptr); // 1st url may be the same as current url
        p->setMedia(items[item_now].url.data());
        p->set(State::Playing);
    });

    glfwSetMouseButtonCallback(win, [](GLFWwindow* win, int button, int action, int mods){
        if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
            return;
        if (gDuration <= 0)
            return;
        double x, y = 0;
        glfwGetCursorPos(win, &x, &y);
        int ww = 0, wh = 0;
        glfwGetWindowSize(win, &ww, &wh);
        if (x < 0 || x > ww)
            return;
        if (y < 0 || y > wh)
            return;
        const int64_t pos = x * gDuration/ww;
        if (pos < 0)
            return;
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        Seek(p, items, pos);
    });
    glfwSetCursorPosCallback(win, [](GLFWwindow* win, double x, double y){
        if (glfwGetKey(win, GLFW_KEY_Z) == GLFW_RELEASE)
            return;
        //printf("cursor: %f %f\n", x, y);
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        if (gDuration <= 0)
            return;
        int ww = 0, wh = 0;
        glfwGetWindowSize(win, &ww, &wh);
        if (x < 0 || x > ww)
            return;
        if (y < 0 || y > wh)
            return;
        const int64_t pos = x * gDuration/ww;
        if (pos < 0)
            return;
        Seek(p, items, pos);
    });
    glfwShowWindow(win);
    player.onStateChanged([=](State s){
    });
    player.setPreloadImmediately(true); // MUST set before setMedia() because setNextMedia() is called when media is changed

    int fw = 0, fh = 0;
    glfwGetFramebufferSize(win, &fw, &fh);
    player.setVideoSurfaceSize(fw, fh);
    //player.setPlaybackRate(2.0f);
    if (!items.empty())
        player.setMedia(items[item_now].url.data());
    if (!cv.empty()) {
        std::regex re(",");
        std::sregex_token_iterator first{cv.begin(), cv.end(), re, -1}, last;
        player.setDecoders(MediaType::Video, {first, last});
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
            return true;
        });
        player.set(State::Playing);
    }

    while (!glfwWindowShouldClose(win)) {
        glfwMakeContextCurrent(win);
        player.renderVideo();
        glfwSwapBuffers(win);
        glfwWaitEvents();
    }
    //player.setVideoSurfaceSize(-1, -1); // it's better to cleanup gl renderer resources in current foreign context. also will release in ~Player()
}
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
