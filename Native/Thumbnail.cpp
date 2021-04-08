#include "mdk/cpp/Player.h"
#include "mdk/cpp/VideoFrame.h"
#include <cstdlib>
#include <cstring>
#include <future>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace MDK_NS;
using namespace std;
int main(int argc, const char** argv)
{
    printf("usage: %s [-c:v DecoderName] [-from milliseconds] [-size widthxheight] file\n", argv[0]);
    VideoFrame v;
    int64_t from = 0;
    int width = -1;
    int height = -1;
    float scale = 1.0f;
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
            printf("error occured!\n");
            pm.set_value(-2);
            return 0;
        }
        // convert and/or copy frame

        if (scale != 1.0 && width == -1) {
            width = v.width() * scale;
            height = v.height() * scale;
        }
        if (width > 0)
            width = (width + 63) & ~63; // stb jpeg assume aligments == 1
        const auto rgb = v.to(PixelFormat::RGB24, width, height);
        printf("decoded @%f. out size: %dx%d, stride: %d, scale: %f\n", v.timestamp(), width, height, rgb.bytesPerLine(), scale);
        stbi_write_jpg("thumbnail.jpg", rgb.width(), rgb.height(), 3, rgb.bufferData(), 80);
        pm.set_value(0);
        return 0;
    });
    p.prepare(from, [&](int64_t pos, bool*){
        if (pos < 0)
            pm.set_value(-1);
        return true;
    });
    p.setState(State::Running);
    const auto ret = fut.get();
    p.setState(State::Stopped);
    return ret;
}