/*
 * Copyright (c) 2016-2018 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with SDL OpenGL example
 */
#include <mdk/Player.h>
#include <SDL.h>
#include <iostream>
#include <cstdlib> // atoi
#include <cstring>
using namespace MDK_NS;
#undef main

int print_help(const char* app) {
    return printf("usage: %s [-from seconds] [-ao audio_renderer] [-c:v video_decoder] url1 [url2 url3 ...]\n", app);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        return print_help(argv[0]);
    SDL_Init(SDL_INIT_VIDEO);
#if 0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // (default) this enables vsync
    SDL_Window *window = SDL_CreateWindow("MDK player with OpenGL Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        800, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_GL_CreateContext(window);
    //setLogHandler(nullptr);
    //setLogHandler(nullptr);
    int url_index = 1;
    int from = 0;
    const char* cv = nullptr;
    const char* ao = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-c:v") == 0) {
            cv = argv[++i];
            url_index += 2;
        } else if (std::strcmp(argv[i], "-from") == 0) {
            from = std::atoi(argv[++i]);
            url_index += 2;
        } else if (std::strcmp(argv[i], "-ao") == 0) {
            ao = argv[++i];
            url_index += 2;
        } else if (argv[i][0] == '-') {
            return print_help(argv[0]);
        }
    }
    int url_now = url_index;
    Player player;
    if (cv)
        player.setVideoDecoders({cv});
    if (ao)
        player.setAudioBackends({ao});
    player.onStateChanged([&](State s){
        printf("state changed to %d, status: %d\n", s, player.mediaStatus());
    });
    player.currentMediaChanged([&]{
        printf("currentMediaChanged %d/%d, now: %s\n", url_now-url_index, argc-url_index, player.url());fflush(stdout);
        if (argc > url_now+1) {
            ++url_now;
            player.setNextMedia(argv[url_now]); // safe to use ref to player
            // alternatively, you can create a custom event
        }
    });
    player.onMediaStatusChanged([](MediaStatus s){
        //MediaStatus s = player.mediaStatus();
        printf("************Media status: %#x, loading: %d, buffering: %d, prepared: %d, EOF: %d**********\n", s, s&MediaStatus::Loading, s&MediaStatus::Buffering, s&MediaStatus::Prepared, s&MediaStatus::End);
        return true;
    });
    //player.setPreloadImmediately(false); // MUST set before setMedia() because setNextMedia() is called when media is changed
    player.setMedia(argv[url_index]);
    player.prepare(from*int64_t(TimeScaleForInt), [](int64_t t, bool*) {
        std::cout << ">>>>>>>>>>>>>>>>>prepared @" << t << std::endl; // FIXME: t is wrong http://qthttp.apple.com.edgesuite.net/1010qwoeiuryfg/sl.m3u8
#if defined(MDK_VERSION_CHECK)
# if MDK_VERSION_CHECK(0, 5, 0)
        return true;
# endif
#endif
    });
    
    player.setState(State::Playing);
//    player.waitFor(State::Playing);
    //player.setState(State::Paused);
    printf("\n~~~~~~~~~~~~~~~state changes to playing~~~~~~~~~~~~~~~\n");
    int w = 1920, h = 1080;
    player.setVideoSurfaceSize(1920, 1080); // ensure surface size is valid when rendering 1st frame (expose event may be too late)
    const Uint32 update_event = SDL_USEREVENT+1;//SDL_RegisterEvents(SDL_USEREVENT+1);
    player.setRenderCallback([=](void*){
        SDL_Event e;
        e.type = update_event;
        SDL_PushEvent(&e);
    });
    player.onEvent([](const MediaEvent& e){
        std::cout << "event: " << e.category << ", detail: " <<e.detail << std::endl;
        return false;
    });
    
    while (true) {
        SDL_Event event;
        SDL_WaitEvent(&event);
        switch (event.type) {
        case SDL_QUIT: goto done;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_EXPOSED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                SDL_GetWindowSize(window, &w, &h);
                player.setVideoSurfaceSize(w, h);
            }
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_SPACE) {
                printf("~~~request state pause: %d\n", player.state() == State::Playing);
                if (player.state() == State::Playing) {
                    player.setState(State::Paused);
                    //player.waitFor(State::Paused);
                } else if (player.state() == State::Paused) {
                    player.setState(State::Playing);
                    //player.waitFor(State::Playing);
                }
                printf("~~~request state end~~~~~\n");
            } else if (event.key.keysym.sym == SDLK_a) {
                static const float ap[] = {
                    IgnoreAspectRatio,
                    KeepAspectRatio,
                    KeepAspectRatioCrop,
                };
                static size_t ai = 0;
                player.setAspectRatio(ap[(ai++)%3]);
            } else if (event.key.keysym.sym == SDLK_o) {
                static int angle = 0;
                player.rotate(angle+=90);
            } else if (event.key.keysym.sym == SDLK_s) {
                player.setState(State::Stopped);
            } else if (event.key.keysym.sym == SDLK_p) {
                player.setState(State::Playing);
            } else if (event.key.keysym.sym == SDLK_RIGHT) {
                printf("seekForward from: %" PRId64 "ms\n", player.position());
                player.seek(player.position()+10*int(TimeScaleForInt),[](int64_t t){
                    printf("seek finished at: %" PRId64 "ms\n", t);
                });
            } else if (event.key.keysym.sym == SDLK_LEFT) {
                printf("seekBackward from: %" PRId64 "\n", player.position());
                player.seek(player.position()-10*int(TimeScaleForInt),[](int64_t t){
                    printf("seek finished at: %" PRId64 "ms\n", t);
                });
            } else if (event.key.keysym.sym == SDLK_UP) {
                const float v = player.playbackRate();
                printf("speed up from: %f to %f++++\n\n", v, v+0.2f);
                player.setPlaybackRate(v + 0.2f);
            } else if (event.key.keysym.sym == SDLK_DOWN) {
                const float v = player.playbackRate();
                printf("speed down from: %f to %f----\n\n", v, v-0.2f);
                player.setPlaybackRate(v - 0.2f);
            } else if (event.key.keysym.sym == SDLK_b) {
                static int index = 1;
                static const int nb_urls = argc - url_index;
                if (nb_urls > 1)
                    player.switchBitrate(argv[(index++)%nb_urls + url_index]);
            }
            break;
        case update_event:
            player.renderVideo();
            SDL_GL_SwapWindow(window);
            break;
        default:
            break;
        }
    }
done:
    Player::foreignGLContextDestroyed();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
