media player examples based on mdk sdk. runs on all platforms. sdk download: https://mdk-sdk.sourceforge.io

About mdk-sdk: https://github.com/wang-bin/mdk-sdk

## GLFW

glfw examples

### glfwplay
- has the most features
- use glfw context as foreign context mode
- use glfw window to create render loop and gl/d3d11 context internally

### multiplayers
1 player per window

### multiwindows
many windows as render targets sharing the same player

### simple
minimal glfw example

## macOS
- use `CAOpenGLLayer` as foreign context
- use NSView to create render loop and gl context internally

## Native

### mdkplay
create platform dependent surface/window internally, also create render loop and gl context

### offscreen
gl offscreen rendering guide

### x11play
Use user provided platform dependent surface/window

## Qt
Integrate with QOpenGLWindow and QOpenGLWidget. Use gl context provided by Qt

## SDL
Use gl context provided by SDL

## SFML
Use gl context provided by SFML

## WindowsStore
Currently only use ANGLE OpenGLES2/3

### CoreWindowCx
- use `Windows::UI::Core::CoreWindow` as platform dependent surface to create render loop and gl context internally
- use App provided gl context

### XamlCx
- Use `Windows::UI::Xaml::Controls::SwapChainPanel` as platform dependent surface to create render loop and gl context internally
- Switch between 2 render targets


## Raspberry Pi

mdk is designed to work in both firmware environment and vc4 environment

### BRCM Firmware Environment
- hardware decoding and 0-copy rendering is fully supported
- run `./mdkplay -c:v MMAL test.mp4` to enable MMAL hardware decoding and OpenGL ES2 zero copy rendering

### VC4/5 environment
- hardware decoding works, 0-copy rendering is impossible in current kernel driver
- run in x11 window (X11 EGL): `LD_PRELOAD=libX11.so ./mdkplay -c:v MMAL test.mp4`
- run in x11 window (GLX): `GL_EGL=0 LD_PRELOAD=libGL.so ./mdkplay -c:v MMAL test.mp4`
- run in wayland (assume weston is running): `./mdkplay -c:v MMAL test.mp4`


## Linux SUNXI

tested on pcdiuno, allwinner a10, sun4i, Linaro 12.11, Linux ubuntu 3.4.29+ #1 PREEMPT Tue Nov 26 15:20:06 CST 2013 armv7l armv7l armv7l GNU/Linux

run:
- [Best performance] cedarv decoder via libvecore, texture uploading is accelerated by UMP:

`./bin/mdkplay -c:v CedarX video_file`

1080p@24fps 7637kb/s h264 decoding + rendering ~28% cpu

- cedarv decoder via libvecore, no UMP accelerated: `GLVA_HOST=1 ./bin/mdkplay -c:v CedarX video_file`

1080p@24fps 7637kb/s h264 decoding + rendering ~96% cpu

- vdpau decoder copy mode: `GLVA_HOST=1 ./bin/mdkplay -c:v VDPAU video_file`

1080p@24fps 7637kb/s h264 decoding + rendering ~97% cpu

- vdpau decoder zero copy mode via nv interop extension(not tested, I have no working vdpau driver): `./bin/mdkplay -c:v VDPAU video_file`

- test decoder speed:

```
    ./bin/framereader -c:v CedarX video_file  # 1080p h264 ~84fps
    ./bin/framereader -c:v FFmpeg video_file  # 1080p h264 ~12fps
```

if default audio device does not sound correctly, try to change the device name via environment var `ALSA_DEVICE`, e.g.

`export ALSA_DEVICE="hw:0,0"`


You don't need to add LD_PRELOAD for qt apps because qt apps are already linked to a certain GL library


