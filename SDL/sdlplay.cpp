/*
 * Copyright (c) 2016-2018 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with QOpenGLWindow example
 */
#include <SDL.h>
#include <cstring>
#include <iostream>
#include <mdk/Player.h>

using namespace MDK_NS;
#undef main
int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("usage: %s url\n", argv[0]);
        return 0;
    }
#ifdef _WIN32
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
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

    int url_index = 1;
    Player player;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-c:v") == 0) {
            player.setVideoDecoders({argv[i+1]});
            url_index += 2;
        }
    }
    //player.setVideoDecoders({"VideoToolbox"});//, "MMAL", "FFmpeg"});
    static int idx = 2;
    bool load_next = false;
    player.onStateChanged([&](State s){
        printf("state changed to %d, status: %d\n", s, player.mediaStatus());
    });
    player.currentMediaChanged([&]{
        //load_next = argc > url_index+1;
        printf("currentMediaChanged %d/%d, now: %s\n", idx, argc, player.url());fflush(stdout);
        //if (argc > idx)
          //  player.setNextMedia(argv[idx++]);
    });
    player.onMediaStatusChanged([&player, window](MediaStatus s){
        //MediaStatus s = player.mediaStatus();
        printf("************Media status: %#x, loading: %d, buffering: %d, EOF: %d**********\n", s, s&LoadingMedia, s&BufferingMedia, s&EndOfMedia);
        return true;
    });
    player.setMedia(argv[url_index]);
    //player.setPreloadImmediately(false);
    //player.prepare(19000, [](int64_t t) {
      //  std::cout << ">>>>>>>>>>>>>>>>>prepared @" << t << std::endl; // FIXME: t is wrong http://qthttp.apple.com.edgesuite.net/1010qwoeiuryfg/sl.m3u8

    //});
    player.setState(State::Playing);
    player.waitFor(State::Playing);
                printf("\n~~~~~~~~~~~~~~~state changes to playing~~~~~~~~~~~~~~~\n");
    static const Uint32 update_event = SDL_RegisterEvents(1);
    player.setRenderCallback([]{
        SDL_Event e;
        e.type = update_event;
        SDL_PushEvent(&e);
    });
    player.addListener([](const MediaEvent& e){
        std::cout << "event: " << e.category << ", detail: " <<e.detail << std::endl;
        return false;
    });
    while (true) {
        SDL_Event event;
        SDL_WaitEvent(&event);
        switch (event.type) {
        case SDL_QUIT:
            goto done;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_EXPOSED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                int w, h;
                SDL_GetWindowSize(window, &w, &h);
                player.setVideoSurfaceSize(w, h);
            }
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_SPACE) {
                printf("~~~request state pause: %d\n", player.state() == State::Playing);
                if (player.state() == State::Playing) {
                    player.setState(State::Paused);
                    player.waitFor(State::Paused);
                } else if (player.state() == State::Paused) {
                    player.setState(State::Playing);
                    player.waitFor(State::Playing);
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
                //player.setOr
            } else if (event.key.keysym.sym == SDLK_s) {
                player.setState(State::Stopped);
            } else if (event.key.keysym.sym == SDLK_p) {
                player.setState(State::Playing);
            } else if (event.key.keysym.sym == SDLK_RIGHT) {
                printf("seekForward from: %" PRId64 "ms\n", player.position());fflush(stdout);
                player.seek(player.position()+10*1000,[](int64_t t){
                    printf("seek finished at: %" PRId64 "ms\n", t);
                });
                // player.seekForward(); // current implmentation seek from buffered position
            } else if (event.key.keysym.sym == SDLK_LEFT) {
                printf("seekBackward from: %" PRId64 "\n", player.position());fflush(stdout);
                player.seek(player.position()-10*1000,[](int64_t t){
                    printf("seek finished at: %" PRId64 "ms\n", t);
                });
                // player.seekBackward(); // current implmentation seek from buffered position
            } else if (event.key.keysym.sym == SDLK_b) {
                static int index = 1;
                static const int nb_urls = argc - url_index;
                if (nb_urls > 1)
                    player.switchBitrate(argv[(index++)%nb_urls + url_index]);
            }
            break;
        default:
            break;
        }
        if (event.type == update_event) {
            player.renderVideo();
            SDL_GL_SwapWindow(window);
        }
        if (load_next) {
            load_next = false;
            player.setNextMedia(argv[idx++]);
        }
    }
done:
//player.setNextMedia(nullptr, -1);
    //player.setState(State::Stopped);
    //player.waitFor(State::Stopped);
    player.destroyRenderer();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
