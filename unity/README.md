# MDK SDK — Unity Plugin Example

A cross-platform Unity plugin that wraps the [MDK SDK](https://github.com/wang-bin/mdk-sdk) and lets you play video on all major Unity platforms using hardware-accelerated decoders and native graphics APIs.

## Supported Platforms

| Platform     | Graphics API          | Hardware Decoder                  |
|-------------|----------------------|-----------------------------------|
| Windows x64  | D3D11 / OpenGL        | D3D11, DXVA, NVDEC, CUDA, FFmpeg  |
| macOS        | Metal / OpenGL        | VideoToolbox, FFmpeg               |
| Linux x64    | OpenGL                | VAAPI, VDPAU, NVDEC, FFmpeg        |
| iOS          | Metal                 | VideoToolbox, FFmpeg               |
| Android      | OpenGL ES 3           | AMediaCodec (MediaCodec), FFmpeg   |

## Directory Layout

```
unity/
├── native/                  # C++ native plugin source
│   ├── MDKPlayer.h          # C API header (for P/Invoke)
│   ├── MDKPlayer.cpp        # C++ implementation
│   └── CMakeLists.txt       # Cross-platform CMake build
└── Assets/
    ├── Plugins/
    │   └── MDK/
    │       └── Scripts/
    │           ├── MDKPlayer.cs       # P/Invoke bindings + managed Player wrapper
    │           └── MDKPlayerView.cs   # MonoBehaviour — renders video to a texture
    └── VideoPlayerController.cs       # Example scene controller with UI hooks
```

## Quick Start

### 1. Download the MDK SDK

Download the nightly MDK SDK for your target platform from  
<https://sourceforge.net/projects/mdk-sdk/files/nightly/>

Extract it so that the directory contains `include/mdk/Player.h` and `lib/cmake/FindMDK.cmake`.

### 2. Build the Native Plugin

```bash
# Desktop (Windows/macOS/Linux) — run from unity/native/
cmake -B build -DMDK_SDK_DIR=/path/to/mdk-sdk
cmake --build build --config Release

# Install into Assets/Plugins (adjust prefix to your Unity project root)
cmake --install build --config Release --prefix ../Assets
```

#### Android (cross-compile from Linux/macOS)

```bash
cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_NATIVE_API_LEVEL=21 \
  -DMDK_SDK_DIR=/path/to/mdk-sdk-android \
  unity/native

cmake --build build-android --config Release
cmake --install build-android --prefix unity/Assets
```

#### iOS (on macOS)

```bash
cmake -B build-ios \
  -GXcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DMDK_SDK_DIR=/path/to/mdk-sdk-ios \
  unity/native

cmake --build build-ios --config Release
cmake --install build-ios --prefix unity/Assets
```

### 3. Place Native Libraries in the Unity Project

After installing, copy the native library into the correct Unity Plugins folder:

| Platform          | File                | Destination in Assets/                                |
|-------------------|---------------------|-------------------------------------------------------|
| Windows x64       | `mdk-unity.dll`     | `Plugins/Windows/x86_64/`                             |
| macOS             | `mdk-unity.bundle`  | `Plugins/macOS/`                                      |
| Linux x64         | `mdk-unity.so`      | `Plugins/Linux/x86_64/`                               |
| iOS               | `mdk-unity.a`       | `Plugins/iOS/`                                        |
| Android arm64-v8a | `libmdk-unity.so`   | `Plugins/Android/libs/arm64-v8a/`                     |

Also copy the MDK SDK runtime library alongside the plugin (e.g. `mdk.dll` on Windows, `libmdk.dylib` on macOS, `libmdk.so` on Linux/Android) into the same folder.

### 4. Set Up a Unity Scene

#### Option A — Inspector

1. Create a `Quad` (or any mesh) in your scene.
2. Add the **MDKPlayerView** component to the Quad.
3. Set the **Url** field to your video path or URL.
4. Optionally add the **VideoPlayerController** component and link the UI elements.
5. Press Play.

#### Option B — Script

```csharp
using MDK;
using UnityEngine;

public class MyVideoPlayer : MonoBehaviour
{
    MDKPlayerView view;

    void Start()
    {
        view = gameObject.AddComponent<MDKPlayerView>();
        view.renderer = GetComponent<Renderer>();
        view.LoopCount = -1; // loop forever
        view.Play("https://example.com/video.mp4");
    }
}
```

## API Overview

### `MDKPlayerView` (MonoBehaviour)

| Member                  | Description                                    |
|-------------------------|------------------------------------------------|
| `Url`                   | Media URL or local file path                   |
| `AutoPlay`              | Auto-play on `Start()`                         |
| `LoopCount`             | -1 = infinite, 0 = no loop                     |
| `Volume`                | Initial volume (0–1)                           |
| `Decoders`              | Comma-separated preferred decoder names        |
| `TargetRawImage`        | UI `RawImage` to display video on              |
| `renderer`              | 3D `Renderer` to display video on              |
| `Play(url)`             | Start/restart playback                         |
| `Pause()`               | Toggle pause                                   |
| `Stop()`                | Stop playback                                  |
| `Seek(ms)`              | Seek to position (milliseconds)                |
| `PlaybackRate`          | Speed multiplier (1.0 = normal)                |
| `Position`              | Current position in ms                         |
| `Duration`              | Total duration in ms                           |
| `OnStateChanged`        | Event fired when playback state changes        |
| `OnMediaStatusChanged`  | Event fired when media status changes          |

### `MDK.Player` (Managed C# Wrapper)

Access via `MDKPlayerView.Player` for lower-level control:

```csharp
var player = playerView.Player;
player.Decoders    = "VideoToolbox,FFmpeg";  // macOS/iOS hardware decode
player.PlaybackRate = 2.0f;                  // 2× speed
player.Volume       = 0.5f;
player.Loop         = -1;
player.Seek(30_000); // jump to 30 s
```

## Notes

- The native plugin must be built separately for each platform.  
  The CMake install step copies it into the correct `Assets/Plugins/` subdirectory.
- On **iOS**, Unity requires static libraries. The build system automatically produces a `.a` file.
- On **Android**, ensure the MDK SDK `.so` (e.g. `libmdk.so`) is also in `Plugins/Android/libs/<abi>/`.
- For **Metal** (macOS/iOS), `MDKPlayer_setMetalRenderTarget` requires a valid `MTLDevice` pointer; the helper `MDKUnity_GetMetalDevice()` stub in `MDKPlayerView.cs` should be implemented in an Objective-C `.mm` file in your project if you need it (Unity Metal integration typically exposes the device via `SystemInfo` or the `UnityMetal` API).
