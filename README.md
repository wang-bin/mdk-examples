# mdk-examples
media player examples based on MDK(Multimedia Development Kit) sdk for all platforms

## Features
- Cross platform: windows desktop, windows store, linux, macOS, iOS, android, raspberry pi
- Hardware accelerated decoding and 0-copy rendering
- OpenGL rendering with/without user provided context
- Ingegrated with any OpenGL gui toolkit (Qt, SDL etc.) easily
- Gapless switch between any videos
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
- run `./mdkplay -c:v MMAL test.mp4` to enable hardware decoding

### VC4/5 environment
- hardware decoding works, 0-copy rendering is impossible in current kernel driver
- run in x11 window (X11 EGL): `LD_PRELOAD=libX11.so ./mdkplay -c:v MMAL test.mp4`
- run in x11 window (GLX): `GL_EGL=0 LD_PRELOAD=libGL.so ./mdkplay -c:v MMAL test.mp4`
- run in wayland (assume weston is running): `./mdkplay -c:v MMAL test.mp4`


You don't need to add LD_PRELOAD for qt apps because qt apps are already linked to a certain GL library


> Wang Bin, wbsecg1@gmail.com

