#include "mdk/Player.h"
#include "mdk/VideoFrame.h"
#include <chrono>
#include <cstring>
#include <future>
#include <queue>

using namespace MDK_NS;
using namespace std;
int main(int argc, const char** argv)
{
    printf("usage: %s [-c:v DecoderName] [-from milliseconds] file\n", argv[0]);
    VideoFrame v;
    int64_t from = 0;
    bool decode1 = false;
    Player p;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-c:v") == 0)
            p.setDecoders(MediaType::Video, {argv[++i]});
        else if (std::strcmp(argv[i], "-from") == 0)
            from = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "-one") == 0) {
            decode1 = true;
        }
    }
    p.setMedia(argv[argc-1]);
    // only the 1st video track will be decoded
    p.setDecoders(MediaType::Audio, {});
    p.onSync([]{return DBL_MAX;}); // do not sync to present time
    std::queue<int64_t> t;
    auto t0 = chrono::system_clock::now();
    int count = 0;
    double fps = 0;
    int64_t elapsed = 1; //1: avoid divided by 0
    promise<int> pm;
    auto fut = pm.get_future();
    p.onFrame<VideoFrame>([&](VideoFrame& v, int){
        if (!v || v.timestamp() == TimestampEOS) { // AOT frame(1st frame, seek end 1st frame) is not valid, but format is valid. eof frame format is invalid
            printf("decoded: %d, elapsed: %" PRId64 ", fps: %.1f/%.1f\n", count, elapsed, count*1000.0/elapsed, fps);
            printf("invalid frame. eof?\n");
            pm.set_value(0);
            return 0;
        }
        if (!v.format()) {
            printf("error occured!\n");
            pm.set_value(-2);
            return 0;
        }
        ++count;
        elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - t0).count();
        t.push(elapsed);
        if (t.size() > 32)
            t.pop();
        fps = double(t.size()*1000)/double(std::max<int64_t>(1LL, t.back() - t.front()));
        printf("decoded @%f: %d, elapsed: %" PRId64 ", fps: %.1f/%.1f, fmt: %d\r", v.timestamp(), count, elapsed, count*1000.0/elapsed, fps, v.format());
        fflush(stdout);
        return 0;
    });
    p.setVideoSurfaceSize(64, 64);
    int rcount = 0;
    p.setRenderCallback([&](void*){
        rcount++;
    });
    p.prepare(from, [&](int64_t pos, bool*){
        if (pos < 0)
            pm.set_value(-1);
        return true;
    });
    p.set(State::Running);
    const auto ret = fut.get();
    printf("frame count: %d, render count: %d\n", count, rcount);
    return ret;
}