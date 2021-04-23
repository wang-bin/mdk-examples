#include "mdk/Player.h"
#include "mdk/MediaInfo.h"
#include <cstdio>
#include <future>
#include <optional>
using namespace MDK_NS;
using namespace std;

int main(int argc, char *argv[])
{
    promise<optional<MediaInfo>> p;
    Player player;
    player.setMedia(argv[argc-1]);
    player.prepare(0LL, [&](int64_t position, bool*){
        if (position < 0) {
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
    return 0;
}
