
/*
 * Copyright (c) 2016-2023 WangBin <wbsecg1 at gmail.com>
 * MDK SDK + GLFW example
 */
#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif
#ifdef _WIN32
#include <d3d11.h>
#endif
#if __has_include(<vulkan/vulkan_core.h>) // FIXME: -I
# include <vulkan/vulkan_core.h>
#endif
#include "mdk/MediaInfo.h"
#include "mdk/RenderAPI.h"
#include "mdk/Player.h"

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

#ifdef _Pragma
# if (GLFW_VERSION_MAJOR > 3 ||  GLFW_VERSION_MINOR > 2)
_Pragma("weak glfwSetWindowContentScaleCallback")
_Pragma("weak glfwGetWindowContentScale")
// libglfw3-wayland has no x11 symbols
_Pragma("weak glfwGetX11Display")
_Pragma("weak glfwGetX11Window")
# endif
#endif

using namespace MDK_NS;

int64_t gSeekStep = 5000LL;
SeekFlag gSeekFlag = SeekFlag::FromStart|SeekFlag::InCache;
int atrack = 0;
int vtrack = 0;
int strack = 0;
int program = -1;

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
            //req.width = -1;
            p->snapshot(&req,
        [](Player::SnapshotRequest* ret, double frameTime){
            return std::to_string(frameTime).append(".jpg");
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
            p->seek(p->position()+gSeekStep, gSeekFlag|SeekFlag::KeyFrame, [](int64_t pos) {
            printf(">>>>>>>>>>seek ret: %lld<<<<<<<<<<<<<<\n", pos);
        }); // Default if GLFW_REPEAT
            break;
        case GLFW_KEY_E:
            p->seek(INT64_MAX, gSeekFlag, [](int64_t pos) {
            printf(">>>>>>>>>>seek ret: %lld<<<<<<<<<<<<<<\n", pos);
        }); // Default if GLFW_REPEAT
            break;
        case GLFW_KEY_LEFT:
            p->seek(p->position()-gSeekStep, gSeekFlag|SeekFlag::KeyFrame, [](int64_t pos) {
            printf(">>>>>>>>>>seek ret: %lld<<<<<<<<<<<<<<\n", pos);
        });
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
            p->record("mdk-record.mov"); // mov supports raw yuv
            break;
        case GLFW_KEY_X:
            p->set(State::Stopped);
            break;
        case GLFW_KEY_G: {
            p->setActiveTracks(MediaType::Unknown, {++program % (int)p->mediaInfo().program.size()});
        }
            break;
        case GLFW_KEY_S:
            p->setActiveTracks(MediaType::Subtitle, {(strack++ % (int)(p->mediaInfo().subtitle.size() + 1)) -1});
            break;
        case GLFW_KEY_A:
            p->setActiveTracks(MediaType::Audio, {(atrack++ % (int)(p->mediaInfo().audio.size() + 1)) - 1});
            break;
        case GLFW_KEY_V:
            p->setActiveTracks(MediaType::Video, {(vtrack++ % (int)(p->mediaInfo().video.size() + 1)) - 1});
            break;
        case GLFW_KEY_Z:
            p->setActiveTracks(MediaType::Video, {});
            break;
        case GLFW_KEY_PERIOD:
            p->seek(1, SeekFlag::FromNow|SeekFlag::Frame, [](int64_t pos) {
            printf(">>>>>>>>>>seek ret: %lld<<<<<<<<<<<<<<\n", pos);
        });
            break;
        case GLFW_KEY_COMMA:
            p->seek(-1, SeekFlag::FromNow|SeekFlag::Frame, [](int64_t pos) {
            printf(">>>>>>>>>>seek ret: %lld<<<<<<<<<<<<<<\n", pos);
        });
            break;
        case GLFW_KEY_P: {
            const ColorSpace cs[] = {ColorSpaceUnknown, ColorSpaceBT709, ColorSpaceBT2100_PQ};
            static int i = 0;
            p->set(cs[i++%std::size(cs)]);
        }
            break;
        case GLFW_KEY_Y: {
            static int cc = 1;
            p->setProperty("cc", to_string(++cc % 2));
        }
            break;
        case GLFW_KEY_U: {
            static int s = 1;
            p->setProperty("subtitle", to_string(++s % 2));
        }
            break;
        default:
            break;
    }
}

void showHelp(const char* argv0)
{
    printf("usage: %s [-d3d11] [-es] [-refresh_rate int_fps] [-c:v decoder] url1 [url2 ...]\n"
            "-bg: background color, 0xrrggbbaa, unorm (r, g, b, a)\n"
            "-colorspace: output color space. can be 'auto'(will enable hdr display on demond if possible), 'bt709', 'bt2100'"
            "-d3d11: d3d11 renderer. support additiona options: -d3d11:feature_level=12.0:debug=1:adapter=0:buffers=2 \n"
            "-gl: use gl renderer. context is created by mdk instead of glfw\n"
            "-metal: metal renderer. support additiona options: -metal:device_index=0 \n"
            "-vk: vulkan renderer. support additiona options: -vk:device_index=0 \n"
            "-logfile: save log to a given file\n"
            "-logLevel or -log: log level name or int value\n"
            "-c:v: video decoder names separated by ','. can be FFmpeg, VideoToolbox, MFT, D3D11, DXVA, NVDEC, CUDA, VDPAU, VAAPI, MMAL(raspberry pi), CedarX(sunxi), MediaCodec\n"
            "a decoder can set property in format 'name:key1=value1:key2=value2'. for example, VideoToolbox:glva=1:hwdec_format=nv12, MFT:d3d=11:pool=1\n"
            "decoder properties(not supported by all): glva=0/1, hwdec_format=..., copy_frame=0/1/-1, hwdevice=...\n"
            "-c:a: audio decoder names separated by ','. can be FFmpeg, MediaCodec. Properties: hwaccel=avcodec_codec_name_suffix(for example at,fixed)\n"
            "-ao: audio renderer. OpenAL, DSound, XAudio2, ALSA, AudioQueue\n"
            "-url:a: individual audio track url.\n"
            "-url:s: individual subtitle track url.\n"
            "-from: start from given seconds\n"
            "-pause: pause at the 1st frame\n"
            "-buffer: buffer duration range in milliseconds, can be 'minMs', 'minMs+maxMs', e.g. -buffer 1000, or -buffer 1000+2000\n"
            "-buffer_drop: drop buffered data when buffered duration exceed max buffer duration. useful for playing realtime streams, e.g. -buffer 0+1000 -buffer_drop to ensure delay < 1s\n"
            "-loop-a: A-B loop A. if not set but -loop-b is set, then A is 0\n"
            "-loop-b: A-B loop B. -1 means end of media. if not set but -loop-a is set, then B is -1\n"
            "-loop: A-B loop repeat count\n"
            "-seek_any: seek to any frame instead of key frame\n"
            "-seek_step: step length(in ms) of seeking forward/backward\n"
            "-autoclose: close when stopped\n" // TODO: check image or video
            "-plugins: plugin names, 'name1:name2...'"
            "-nosync: render video ASAP.\n"
            "-t:a: audio track number. default 0\n"
            "-t:v: video track number. default 0\n"
            "-t:s: subtitle track number. default 0\n"
            "-program: program number(for medias contain multiple programs)\n"
            "Keys:\n"
            "space: pause/resume\n"
            "left/right: seek backward/forward (-/+10s)\n"
            "right: seek forward (+10s)\n"
            "up/down: speed +/-0.1\n"
            "C: capture video frame and save as N.jpg\n"
            "\n"
            "D: switch video decoder\n"
            "F: fullscreen\n"
            "L: stop looping\n"
            "O: change orientation\n"
            "Q: quit\n"
            "R: record video as 'mdk-record.mkv', press again to stop recording\n"
            ".: 1 frame step forward\n"
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
    SetGlobalOption("MDK_KEY", "92178446AF0885458A93CDF446E3B9160A5FC865796E9FCFF262D534389866D04BD4FA52EFECF1BF7E14B0D73A5E8C493A06876FF5BEDC6F801A46B42E7873026DE87BB9AF087ABA756C320BB91C46E94A5FC0021508E8BF9E03ACD25AB0539D4EA194B0D543B5179056FC62441CB248878AF87D3B72ACF6B9F753BA59187A02");
{
    bool help = argc < 2;
    bool es = false;
    bool autoclose = false;
    int from = 0;
    float wait = 0;
    float speed = 1.0f;
    int64_t buf_min = -1;
    int64_t buf_max = -1;
    int loop = -2;
    int64_t loop_a = -1;
    int64_t loop_b = 0;
    bool buf_drop = false;
    bool pause = false;
    bool nosync = false;
    const char* urla = nullptr;
    const char* urlsub = nullptr;
    string vendor;
    std::string ca, cv;
    // some plugins must be loaded before creating player
    for (int i = 1; i < argc; ++i) { // "plugins_dir" and "plugins" MUST be set before player constructed
        if (std::strncmp(argv[i], "-plugins", strlen("-plugins")) == 0) {
            SetGlobalOption(argv[i] + 1, (const char*)argv[++i]);
            break;
        }
    }

    Player player;
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
    //player.setAspectRatio(-1.0);
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-bg") == 0) {
            auto c = std::stoll(argv[++i], nullptr, 16);
            const auto r = c >> 24;
            const auto g = (c >> 16) & 0xff;
            const auto b = (c >> 8) & 0xff;
            const auto a = c & 0xff;
            player.setBackgroundColor(float(r)/255.0, float(g)/255.0, float(b)/255.0, float(a)/255.0);
        } else if (strcmp(argv[i], "-colorspace") == 0) {
            auto cs = argv[++i];
            if (strcmp(cs, "auto") == 0) {
                player.set(ColorSpaceUnknown);
            } else if (strcmp(cs, "bt709") == 0) {
                player.set(ColorSpaceBT709);
            } else if (strcmp(cs, "bt2100") == 0) {
                player.set(ColorSpaceBT2100_PQ);
            }
        } else if (strcmp(argv[i], "-c:v") == 0) {
            cv = argv[++i];
        } else if (strcmp(argv[i], "-c:a") == 0) {
            ca = argv[++i];
        } else if (strcmp(argv[i], "-t:a") == 0) {
            atrack = std::atoi(argv[++i]);
            if (atrack >= 0)
                player.setActiveTracks(MediaType::Audio, {atrack});
            else
                player.setActiveTracks(MediaType::Audio, {});
        } else if (strcmp(argv[i], "-t:v") == 0) {
            vtrack = std::atoi(argv[++i]);
            if (vtrack >= 0)
                player.setActiveTracks(MediaType::Video, {vtrack});
            else
                player.setActiveTracks(MediaType::Video, {});
        } else if (strcmp(argv[i], "-t:s") == 0) {
            strack = std::atoi(argv[++i]);
            if (strack >= 0)
                player.setActiveTracks(MediaType::Subtitle, {strack});
            else
                player.setActiveTracks(MediaType::Subtitle, {});
        } else if (strcmp(argv[i], "-program") == 0) {
            program = std::atoi(argv[++i]);
            if (program)
                player.setActiveTracks(MediaType::Unknown, {program});
        } else if (strstr(argv[i], "-d3d11") == argv[i]) {
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
        } else if (strcmp(argv[i], "-es") == 0) {
            es = true;
        } else if (std::strcmp(argv[i], "-from") == 0) {
            from = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-pause") == 0) {
            pause = true;
        } else if (std::strcmp(argv[i], "-ao") == 0) {
            player.setAudioBackends({argv[++i]});
        } else if (std::strcmp(argv[i], "-fps") == 0) {
            player.setFrameRate(std::stof(argv[++i]));
        } else if (std::strcmp(argv[i], "-refresh_rate") == 0) {
            wait = 1.0f/atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-url:a") == 0) {
            urla = argv[++i];
        } else if (std::strcmp(argv[i], "-url:s") == 0) {
            urlsub = argv[++i];
        } else if (std::strcmp(argv[i], "-buffer") == 0) {
            char *s = (char *)argv[++i];
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
            speed = (float)atof(argv[++i]);
        } else if (std::strcmp(argv[i], "-seek_any") == 0) {
            gSeekFlag = SeekFlag::FromStart;
        } else if (std::strcmp(argv[i], "-seek_step") == 0) {
            gSeekStep = atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-plugins") == 0) {
            SetGlobalOption("plugins", argv[++i]);
        } else if (std::strcmp(argv[i], "-autoclose") == 0) {
            autoclose = true;
        } else if (std::strcmp(argv[i], "-nosync") == 0) {
            nosync = true;
        } else if (std::strcmp(argv[i], "-record") == 0) {
            player.record("record.mkv"); // FIXME: record before play is not supported yet
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
            for (int j = i; j < argc; ++j)
                urls.emplace_back(argv[j]);
            break;
        }
    }
    //SetGlobalOption("videoout.clear_on_stop", 0);
    //auto libavformat = dlopen("libavformat.58.dylib", RTLD_LOCAL);
    //SetGlobalOption("avformat", libavformat);
    if (help)
        showHelp(argv[0]);
    if (ra)
        player.setRenderAPI(ra);
    if (speed != 1.0f)
        player.setPlaybackRate(speed);
//player.setProperty("continue_at_end", "1");
    //player.setProperty("video.avfilter", "format=pix_fmts=yuv420p10be");
    if ((buf_min >= 0 || buf_max >= 0) || buf_drop)
        player.setBufferRange(buf_min, buf_max, buf_drop);
    player.currentMediaChanged([&]{
        std::printf("currentMediaChanged %d/%zu, now: %s\n", url_now, urls.size(), player.url());fflush(stdout);
        const auto now_idx = url_now;
        url_now++;
        if (loop == -1)
            url_now %= urls.size();
        if (urls.size() > url_now && now_idx != url_now) {
            player.setNextMedia(urls[url_now].data()); // safe to use ref to player
            // alternatively, you can create a custom event
        }
    });
    player.onMediaStatusChanged([](MediaStatus s){
        //MediaStatus s = player.mediaStatus();
        printf("************Media status: %#x, invalid: %#x, loading: %d, unloaded: %d, buffering: %d, seeking: %#x, prepared: %d, EOF: %d**********\n", s, s&MediaStatus::Invalid, s&MediaStatus::Loading, s&MediaStatus::Unloaded, s&MediaStatus::Buffering, s&MediaStatus::Seeking, s&MediaStatus::Prepared, s&MediaStatus::End);
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
    if (nosync)
        player.onSync([]{return DBL_MAX;});
    if (!ra && wait <= 0)
        player.setRenderCallback([](void*){
            glfwPostEmptyEvent(); // FIXME: some events are lost on macOS. glfw bug?
        });

    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "Error: %s\n", description);
    });
    if (!glfwInit())
        exit(EXIT_FAILURE);
    //glfwWindowHint(GLFW_SCALE_TO_MONITOR, 1);
    if (es && !ra) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    }
    if (ra) { // we create gl context ourself, glfw should provide a clean window.
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
        float xscale = 1.0f, yscale = 1.0f;
#if (GLFW_VERSION_MAJOR > 3 ||  GLFW_VERSION_MINOR > 2)
    if (glfwGetWindowContentScale)
        glfwGetWindowContentScale(win, &xscale, &yscale);
#endif
        printf("************framebuffer size changed: %dx%d, scale: %f***********\n", w, h, xscale);
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
        string subtitle;
        urls.clear();
        for (int i = 0; i < count; ++i) {
            if (const auto dot = string_view(files[i]).find_last_of('.'); dot != string_view::npos) {
                string ext = files[i] + dot + 1;
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){
                    return std::tolower(c);
                });
                if (ext == "ass" || ext == "ssa" || ext == "srt" || ext == "vtt" || ext == "txt" || ext == "sup" || ext == "sub" || ext == "lrc" || ext == "scc" || ext == "smi" || ext == "sami" || ext == "pjs" || ext == "mpl2" || ext == "rt") {
                    subtitle = files[i];
                } else {
                    urls.emplace_back(files[i]);
                }
            }
        }
        if (urls.empty()) {
            if (!subtitle.empty())
                p->setMedia(subtitle.data(), MediaType::Subtitle);
            return;
        }

        p->setNextMedia(nullptr);
        p->set(State::Stopped);
        p->waitFor(State::Stopped);
        p->setMedia(nullptr); // 1st url may be the same as current url
        url_now = 0;
        p->setMedia(urls[url_now].data());
        if (!subtitle.empty())
            p->setMedia(subtitle.data(), MediaType::Subtitle); // MUST be after setMedia(url) if url changed

        p->set(State::Playing);
    });
    glfwShowWindow(win);
#if defined(GLFW_EXPOSE_NATIVE_X11)
    if (glfwGetX11Display)
        setNativeDisplay(glfwGetX11Display());
#endif
    player.onStateChanged([=](State s){
        if (s == State::Stopped && autoclose) {
            glfwSetWindowShouldClose(win, 1);
            glfwPostEmptyEvent();
        }
    });
    glfwSetMouseButtonCallback(win, [](GLFWwindow* win, int button, int action, int mods){
        if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
            return;
        double x, y = 0;
        glfwGetCursorPos(win, &x, &y);
        int w = 0, h = 0;
        glfwGetWindowSize(win, &w, &h);
        if (x > w || y > h)
            return;
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        auto duration = p->mediaInfo().duration;
        p->seek(duration*int(x)/w, gSeekFlag, [](int64_t pos) {
            printf(">>>>>>>>>>seek ret: %lld<<<<<<<<<<<<<<\n", pos);
        }); // duration*x/w crashes clang-cl-11/12 -Oz
    });
    glfwSetCursorPosCallback(win, [](GLFWwindow* win, double x,double y){
        auto p = static_cast<Player*>(glfwGetWindowUserPointer(win));
        if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
            return;
        int w = 0, h = 0;
        glfwGetWindowSize(win, &w, &h);
        if (x > w || y > h)
            return;
        auto duration = p->mediaInfo().duration;
        p->seek(duration*int(x)/w, gSeekFlag, [](int64_t pos) {
            printf(">>>>>>>>>>seek ret: %lld<<<<<<<<<<<<<<\n", pos);
        }); // duration*x/w crashes clang-cl-11/12 -Oz
    });
    player.setPreloadImmediately(true); // MUST set before setMedia() because setNextMedia() is called when media is changed
    float xscale = 1.0f, yscale = 1.0f;
#if (GLFW_VERSION_MAJOR > 3 ||  GLFW_VERSION_MINOR > 2)
    if (glfwGetWindowContentScale)
        glfwGetWindowContentScale(win, &xscale, &yscale);
#endif
// setDecoders before setMedia(), setNextMedia() in currentMediaChanged() callback may load immediately

    int fw = 0, fh = 0;
    glfwGetFramebufferSize(win, &fw, &fh);
    printf("************fb size %dx%d, requested size: %dx%d, scale= %fx%f***********\n", fw, fh, w, h, xscale, yscale);
    player.setVideoSurfaceSize(fw, fh);
    //player.setPlaybackRate(2.0f);
    if (!urls.empty()) {
#ifdef _WIN32
        filesystem::path p(urls[url_now].data());
        player.setMedia((char*)p.u8string().data());
#else
        player.setMedia(urls[url_now].data());
#endif
    }
    if (urla) {
        player.setMedia(urla, MediaType::Audio);
    }
    if (urlsub) {
        player.setMedia(urlsub, MediaType::Subtitle);
    }
    if (!cv.empty()) {
        std::regex re(",");
        std::sregex_token_iterator first{cv.begin(), cv.end(), re, -1}, last;
        player.setDecoders(MediaType::Video, {first, last});
    }
    if (!ca.empty()) {
        std::regex re(",");
        std::sregex_token_iterator first{ca.begin(), ca.end(), re, -1}, last;
        player.setDecoders(MediaType::Audio, {first, last});
    }

    if (loop >= -1 && urls.size() == 1)
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
        if (!pause)
            player.set(State::Playing);
    }

    if (ra) {
        auto surface_type = MDK_NS::Player::SurfaceType::Auto;
#if defined(_WIN32)
        auto hwnd = glfwGetWin32Window(win);
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
        auto hwnd = glfwGetCocoaWindow(win);
#elif defined(GLFW_EXPOSE_NATIVE_X11)
        Window hwnd = 0;
        if (glfwGetX11Window)
            hwnd = glfwGetX11Window(win);
        surface_type = MDK_NS::Player::SurfaceType::X11;
#endif
        player.updateNativeSurface((void*)hwnd, -1, -1, surface_type);
        //player.showSurface(); // let glfw process events. event handling in mdk is only implemented in win32 and x11 for now
        //exit(EXIT_SUCCESS);
    }
    //float vr[] = {0, 0, 0.5f, 0.5f};
    //player.setPointMap(vr);
    while (!glfwWindowShouldClose(win)) {
        if (!ra) {
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
    if (!ra) // will release internally if use native surface
        player.setVideoSurfaceSize(-1, -1); // it's better to cleanup gl renderer resources in current foreign context
}
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
