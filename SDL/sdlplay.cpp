/*
 * Copyright (c) 2016-2018 WangBin <wbsecg1 at gmail.com>
 * MDK SDK with SDL OpenGL example
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
        printf("usage: %s [-c:v decoder] url\n", argv[0]);
        return 0;
    }
#ifdef _WIN32
    CoInitializeEx(nullptr, COINIT_MULTITHREADED); // for xaudio2 mt
#endif
    SDL_Init(SDL_INIT_VIDEO);
#if 0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // (default) this enables vsync
    SDL_Window *window = SDL_CreateWindow("MDK player + SDL OpenGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
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
    static int idx = 2;
    bool load_next = false;
    player.onStateChanged([&](State s){ printf("state changed to %d, status: %d\n", s, player.mediaStatus());});
    player.onMediaStatusChanged([&player, window](MediaStatus s){
        printf("************Media status: %#x, loading: %d, buffering: %d, EOF: %d**********\n", s, s&LoadingMedia, s&BufferingMedia, s&EndOfMedia);
        return true;
    });
    player.setMedia(argv[url_index]);
    //player.prepare(19000, [](int64_t t) {std::cout << "prepared @" << t << std::endl;});
    player.setState(State::Playing);
//    player.waitFor(State::Playing);
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
        case SDL_QUIT: goto done;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_EXPOSED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                int w, h;
                SDL_GetWindowSize(window, &w, &h);
                player.setVideoSurfaceSize(w, h);
            }
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_SPACE)
                player.setState(player.state() == State::Playing ? State::Paused : State::Playing);
            else if (event.key.keysym.sym == SDLK_RIGHT)
                player.seek(player.position()+10*1000,[](int64_t t){
                    printf("seekForward finished at: %" PRId64 "ms\n", t);
                });
            else if (event.key.keysym.sym == SDLK_LEFT)
                player.seek(player.position()-10*1000);
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
    player.destroyRenderer();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}