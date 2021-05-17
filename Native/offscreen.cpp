#include "mdk/Player.h"
#include "mdk/MediaInfo.h"
#include <cstring>
#include <cstdio>
#include <future>
#define MEDIAINFO_IN_PREPARE 0 // or 1
using namespace MDK_NS;
using namespace std;

int main(int argc, char *argv[])
{
    promise<void> p;
    Player player;
    MediaStatus st = MediaStatus::NoMedia;
    // set callbacks before play
    player.onStateChanged([&p](State s){
        if (s == State::Stopped)
            p.set_value();
    })
#if !MEDIAINFO_IN_PREPARE
    .onMediaStatusChanged([&](MediaStatus s){
        if (flags_added(st, s, MediaStatus::Loaded)) {
            auto& c = player.mediaInfo().video[0].codec;
            player.setVideoSurfaceSize(c.width, c.height);
            printf("notify your opengl fbo is ready to resize %dx%d\n", c.width, c.height);
        }
        st = s;
        return true;
    })
#endif
    .setRenderCallback([](void*){
        printf("notify your opengl context is ready to call player.renderVideo() or directly call it\n");
    })
    ;

    player.setMedia(argv[argc-1]);
#if MEDIAINFO_IN_PREPARE
    player.prepare(0LL, [&](int64_t position, bool*){
        if (position < 0) {
            p.set_value();
            return false;
        }
        auto& c = player.mediaInfo().video[0].codec;
        player.setVideoSurfaceSize(c.width, c.height);
        printf("notify your opengl fbo is ready to resize %dx%d\n", c.width, c.height);
        return true;
    });
#endif
    player.set(State::Playing);
    auto fut = p.get_future();
    fut.wait();
    return 0;
}
