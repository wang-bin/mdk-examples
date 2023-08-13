#include "mdk/Player.h"
#include "mdk/VideoFrame.h"
//#include "mdk/MediaInfo.h"
#include <chrono>
#include <cstring>
#include <future>
#include <queue>
// FIXME: asan crash

using namespace MDK_NS;
using namespace std;
int main(int argc, const char** argv)
{
    printf("usage: %s [-c:v DecoderName] [-from milliseconds] file\n", argv[0]);

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-plugins") == 0) {
            SetGlobalOption("plugins", argv[++i]);
            break;
        }
    }

    const VideoFrame v;
    int64_t from = 0;
    bool decode1 = false;
    int loop = 0, l = 0;
    Player p;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-c:v") == 0)
            p.setDecoders(MediaType::Video, {argv[++i]});
        else if (std::strcmp(argv[i], "-from") == 0)
            from = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "-one") == 0)
            decode1 = true;
        else if (std::strcmp(argv[i], "-loop") == 0)
            loop = std::atoi(argv[++i]);
    }
    p.setMedia(argv[argc-1]);
    p.setLoop(loop);
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
            l += v && v.timestamp() == TimestampEOS;
            printf("invalid frame %d. eof? loop=%d/%d\n", v.isValid(), l, loop);
            if (loop >= 0 && l > loop) {
                pm.set_value(0);
            }
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
    //p.waitFor(State::Paused);
    //printf("video format: %d - %s\n", p.mediaInfo().video[0].codec.format, p.mediaInfo().video[0].codec.format_name);
    p.set(State::Running);
    //this_thread::sleep_for(1s);
    //p.set(State::Stopped);
    const auto ret = fut.get();
    printf("frame count: %d, render count: %d\n", count, rcount);
    //p.waitFor(State::Stopped);
    return ret;
}