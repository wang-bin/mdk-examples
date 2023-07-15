#include "mdk/Player.h"
#include "mdk/MediaInfo.h"
#include <cinttypes>
#include <cstdio>
#include <future>
#include <optional>
using namespace MDK_NS;
using namespace std;

// loading a media is async. 3 ways to get media info when media is loaded
void test_status_future(const char* url)
{
    promise<optional<MediaInfo>> p;
    MediaStatus st = MediaStatus::NoMedia;
    Player player;
    player.onMediaStatusChanged([&](MediaStatus s){
        if (flags_added(st, s, MediaStatus::Loaded)) {
            p.set_value(player.mediaInfo());
        } else if (flags_added(st, s, MediaStatus::Invalid)) {
            p.set_value(nullopt);
        }
        st = s;
        return false; // unload immediately
    });
    player.setMedia(url);
    player.prepare();
    auto fut = p.get_future();
    auto info = fut.get();
    if (info) {
        printf("duration: %" PRId64 "ms\n", info->duration);
    }
}

void test_prepare_future(const char* url, int64_t from = 0LL)
{
    promise<optional<MediaInfo>> p;
    Player player;
    player.setMedia(url);
    // disable tracks, no decode
    player.setActiveTracks(MediaType::Audio, {});
    player.setActiveTracks(MediaType::Video, {});
    player.prepare(from, [&](int64_t position, bool*){
        if (position < 0) { // open error
            p.set_value(nullopt);
            return false;
        }
        p.set_value(player.mediaInfo());
        return false;
    });
    auto fut = p.get_future();
    auto info = fut.get();
    if (info) {
        printf("duration: %" PRId64 "ms\n", info->duration);
    }
}

void test_state(const char* url)
{
    Player player;
    player.setMedia(url);
    // disable tracks, no decode
    player.setActiveTracks(MediaType::Audio, {});
    player.setActiveTracks(MediaType::Video, {});
    player.prepare(); // will try to demux some packets and decode the first frame if any track is enabled
    player.waitFor(State::Paused);
    const auto duration = player.mediaInfo().duration;
    printf("duration: %" PRId64 "ms\n", duration);
}

int main(int argc, char *argv[])
{
    test_status_future(argv[argc - 1]);
    test_prepare_future(argv[argc - 1], 90LL);
    test_state(argv[argc - 1]);
    return 0;
}
