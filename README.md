# mdk-examples
media player examples based on MDK(Multimedia Development Kit) sdk for all platforms

## Features
- Cross platform: windows desktop, windows store, linux, macOS, iOS, android, raspberry pi
- Hardware accelerated decoding and 0-copy rendering
- OpenGL rendering with/without user provided context
- Ingegrated with any OpenGL gui toolkit (Qt, SDL etc.) easily
- Many more

## Examples

## SDK Libraries
- libmdk.so.0|libmdk.dll|mdk.dll: mdk framework runtime library
- libmdk.so|libmdk.a|mdk.lib: mdk framework development library
- mdk.framework: mdk framework for apple
- libmdk-avglue.so|dll|dylib: mdk ffmpeg glue library
- *.dsym: debug symbol file

## Raspberry Pi

mdk is designed to work in both firmware environment and vc4 environment

### bcm firmware environment
- hardware decoding and 0-copy rendering is fully supported
- run `LD_PRELOAD=libbrcmGLESv2.so ./mdkplay -c:v MMAL test.mp4` to enable hardware decoding
- mdk does not directly link to libGLESv2, so you may need to load it manually(e.g. use LD_PRELOAD) if your app does not link to libGLESv2.

### VC4 environment
- hardware decoding works, 0-copy rendering is impossible in current kernel driver
- run in x11 window: `LD_PRELOAD=libX11.so:/usr/lib/arm-linux-gnueabihf/libGLESv2.so ./mdkplay -c:v MMAL test.mp4`
- run in wayland: `LD_PRELOAD=/usr/lib/arm-linux-gnueabihf/libGLESv2.so ./mdkplay -c:v MMAL test.mp4`


You don't need to add LD_PRELOAD for qt apps because qt apps are already linked to a certain GL library


> Wang Bin, wbsecg1@gmail.com

