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


