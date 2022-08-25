#include "mdk/Player.h"
#include "mdk/VideoFrame.h"
#include "mdk/MediaInfo.h"
#include <cstdlib>
#include <cstring>
#include <future>

using namespace MDK_NS;
using namespace std;
int main(int argc, const char** argv)
{
    printf("usage: %s [-c:v DecoderName] [-from milliseconds] [-size widthxheight] [-scale value] [-raw] file\n", argv[0]);
    VideoFrame v;
    int64_t from = 0;
    int width = -1;
    int height = -1;
    float scale = 1.0f;
    bool raw = false;
    Player p;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-c:v") == 0)
            p.setDecoders(MediaType::Video, {argv[++i]});
        else if (std::strcmp(argv[i], "-from") == 0)
            from = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "-size") == 0) {
            auto s = (char*)argv[++i];
            width = strtol(s, &s, 10);
            height = strtol(s+1, nullptr, 10);
        } else if (std::strcmp(argv[i], "-scale") == 0) {
            scale = atof(argv[++i]);
        } else if (std::strcmp(argv[i], "-raw") == 0) {
            raw = true;
        }
    }
    p.setMedia(argv[argc-1]);
    // only the 1st video track will be decoded
    p.setDecoders(MediaType::Audio, {});
    p.onSync([]{return DBL_MAX;}); // do not sync to present time
    promise<int> pm;
    auto fut = pm.get_future();
    bool decoded = false;
    p.onFrame<VideoFrame>([&](VideoFrame& v, int){
        if (decoded)
            return 0;
        decoded = true;
        if (!v || v.timestamp() == TimestampEOS) { // AOT frame(1st frame, seek end 1st frame) is not valid, but format is valid. eof frame format is invalid
            printf("invalid frame. eof?\n");
            pm.set_value(0);
            return 0;
        }
        if (!v.format()) {
            printf("unsupported format in c api!\n");
            pm.set_value(-2);
            return 0;
        }
        // convert and/or copy frame

        if (scale != 1.0 && width == -1) {
            width = v.width() * scale;
            height = v.height() * scale;
        }
        if (raw) {
            printf("decoded @%f. out size: %dx%d, stride: %d, scale: %f\n", v.timestamp(), width, height, v.bytesPerLine(), scale);
            v.save("thumbnail");
        } else if (scale == 1.0f) {
            printf("decoded @%f. out size: %dx%d, stride: %d, scale: %f\n", v.timestamp(), width, height, v.bytesPerLine(), scale);
            v.save("thumbnail0.png");
        } else {
            const auto rgb = v.to(PixelFormat::RGBA, width, height);
            printf("decoded @%f. out size: %dx%d, stride: %d, scale: %f\n", v.timestamp(), width, height, rgb.bytesPerLine(), scale);
            rgb.save("thumbnail.png");
        }
        pm.set_value(v.timestamp() * 1000.0);
        return 0;
    });
    p.prepare(from, [&](int64_t pos, bool*){
        if (pos < 0)
            pm.set_value(-1);
        if (p.mediaInfo().video.empty())
            pm.set_value(-2);
        return true;
    });
    const auto ret = fut.get();
    printf("ret: %d\n", ret);
    return 0;
}