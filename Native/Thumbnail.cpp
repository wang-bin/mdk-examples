#include "mdk/Player.h"
#include "mdk/VideoFrame.h"
#include "mdk/MediaInfo.h"
#include <cstdlib>
#include <cstring>
#include <regex>

using namespace MDK_NS;
using namespace std;
int main(int argc, const char** argv)
{
    printf("usage: %s [-c:v DecoderName] [-from milliseconds/percent_in_float] [-size widthxheight] [-scale value] [-raw] file\n", argv[0]);
    int64_t from = 0;
    float from_percent = 0;
    int width = -1;
    int height = -1;
    float scale = 1.0f;
    bool raw = false;
    int ret = 0; // before player object, destroyed later than player, ret may be accessed(in callback) in player dtor
    Player p;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-c:v") == 0) {
            string cv = argv[++i];
            std::regex re(",");
            std::sregex_token_iterator first{cv.begin(), cv.end(), re, -1}, last;
            p.setDecoders(MediaType::Video, {first, last});
        } else if (std::strcmp(argv[i], "-from") == 0) {
            auto f = argv[++i];
            if (strchr(f, '.'))
                from_percent = std::atof(f);
            else
                from = std::atoi(f);
        } else if (std::strcmp(argv[i], "-size") == 0) {
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
    p.setActiveTracks(MediaType::Audio, {});
    p.setActiveTracks(MediaType::Subtitle, {});
    p.onSync([]{return DBL_MAX;}); // do not sync to present time
    bool decoded = false;
    p.onFrame<VideoFrame>([&](VideoFrame& v, int){
        if (decoded)
            return 0;
        if (v.timestamp() == TimestampEOS) { // eof frame format is invalid
            if (ret == 0) { // -3: seek error
                ret = decoded ? 0 : -1;
            }
            p.set(State::Stopped);
            return 0;
        }
        if (!v) { // an invalid frame is sent before/after seek, and before the 1st frame
            return 0;
        }
        decoded = true;
        // convert and/or copy frame

        if (scale != 1.0 && width == -1) {
            width = v.width() * scale;
            height = v.height() * scale;
        }
        if (raw) {
            printf("decoded @%f. out size: %dx%d, stride: %d, scale: %f. save raw\n", v.timestamp(), width, height, v.bytesPerLine(), scale);
            v.save("thumbnail");
        } else if (scale == 1.0f) {
            printf("decoded @%f. out size: %dx%d, stride: %d, scale: %f\n", v.timestamp(), width, height, v.bytesPerLine(), scale);
            v.save("thumbnail0.png");
        } else {
            const auto rgb = v.to(PixelFormat::RGBA, width, height);
            printf("decoded @%f. out size: %dx%d, stride: %d, scale: %f\n", v.timestamp(), width, height, rgb.bytesPerLine(), scale);
            rgb.save("thumbnail.png");
        }
        ret = v.timestamp() * 1000.0;
        p.set(State::Stopped);
        return 0;
    });
    p.prepare(from, [&](int64_t pos, bool*){
        if ((pos < 0 || p.mediaInfo().video.empty())) {
            ret = -2;
        } else if (from_percent > 0 && from_percent <= 1.0) {
            p.seek(p.mediaInfo().duration * from_percent
                , SeekFlag::FromStart|SeekFlag::KeyFrame|SeekFlag::Backward // KeyFrame: thumbnail should be fast. backward: avoid EPERM error
                , [&](int64_t pos) {
                    if (pos < 0 ) {
                        ret = -3;
                        p.set(State::Stopped);
                    }
                });
        }
        return true;
    });
    p.waitFor(State::Paused); // initial state is Stopped
    p.waitFor(State::Stopped);
    printf("ret: %d\n", ret);
    return 0;
}