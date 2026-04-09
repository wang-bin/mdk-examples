# MDK Player – OHOS (OpenHarmony) Example

A media player example for [OpenHarmony](https://www.openharmony.cn/) (OHOS / HarmonyOS) using the [MDK SDK](https://github.com/wang-bin/mdk-sdk).

## Features

- **XComponent** native surface for hardware-accelerated video rendering via `player.updateNativeSurface()`
- **Local file playback** using the system Document Picker; the selected file is opened as a file descriptor and passed to the player with the `fd://` protocol (`player.setMedia("fd://N")`)
- **Network URL playback** – HTTP, HTTPS, RTSP, RTMP, HLS, DASH, etc.
- **Progress slider** – shows current position and allows seeking
- **Play / Pause / Stop** buttons
- **Playback speed** selector (0.25×–2.0×)
- **Volume slider**

## Project Structure

```
ohos/
├── AppScope/                         # App-level resources & config
│   ├── app.json5
│   └── resources/base/element/
│       └── string.json
├── entry/
│   ├── src/main/
│   │   ├── cpp/
│   │   │   ├── CMakeLists.txt        # Native build script
│   │   │   ├── player_napi.cpp       # N-API module (MDK + XComponent glue)
│   │   │   └── types/libplayer_napi/
│   │   │       ├── index.d.ts        # TypeScript declarations
│   │   │       └── oh-package.json5
│   │   ├── ets/
│   │   │   ├── entryability/
│   │   │   │   └── EntryAbility.ets
│   │   │   └── pages/
│   │   │       └── Index.ets         # Main UI (XComponent + controls)
│   │   ├── resources/
│   │   │   └── base/
│   │   │       ├── element/
│   │   │       │   ├── color.json
│   │   │       │   └── string.json
│   │   │       ├── media/            # Place icon.png here
│   │   │       └── profile/
│   │   │           └── main_pages.json
│   │   └── module.json5
│   ├── build-profile.json5
│   ├── hvigorfile.ts
│   └── oh-package.json5
├── build-profile.json5
├── hvigorfile.ts
└── oh-package.json5
```

## Prerequisites

- [DevEco Studio](https://developer.huawei.com/consumer/cn/deveco-studio/) 5.x (supports API 12 / OHOS 5.0)
- OHOS MDK SDK – download from <https://sourceforge.net/projects/mdk-sdk/files/nightly/>

## Getting the MDK SDK

1. Download the OHOS MDK SDK (e.g. `mdk-sdk-ohos.tar.xz`).
2. Extract it so the directory layout is:

```
ohos/entry/src/main/cpp/mdk-sdk/
├── include/
│   └── mdk/
│       ├── Player.h
│       └── ...
└── lib/
    └── ohos/
        ├── arm64-v8a/
        │   └── libmdk.so
        └── x86_64/
            └── libmdk.so
```

The path is configured in `entry/src/main/cpp/CMakeLists.txt` via the `MDK_SDK_DIR` variable (default: `cpp/mdk-sdk`).

## Building

1. Open the `ohos/` directory in **DevEco Studio**.
2. Sync the project (`File → Sync Project with hvigor Files`).
3. Connect an OHOS device or start an emulator.
4. Click **Run** (▶).

To build from the command line:
```bash
cd ohos
hvigorw assembleHap --mode project -p product=default
```

## How It Works

### Native Surface (XComponent)

The `XComponent` in `Index.ets` specifies `libraryname: 'player_napi'`.  
When the component is rendered, OHOS loads `libplayer_napi.so` and calls `Init()`, which:

1. Extracts the `OH_NativeXComponent*` handle from the module exports.
2. Creates an `MDK_NS::Player` instance keyed by the XComponent ID.
3. Registers surface lifecycle callbacks:
   - `OnSurfaceCreated` → `player.updateNativeSurface(window, w, h)` — attaches MDK to the native window.
   - `OnSurfaceChanged` → `player.setVideoSurfaceSize(w, h)` — handles resize.
   - `OnSurfaceDestroyed` → `player.updateNativeSurface(nullptr, 0, 0)` — detaches.

### File Playback (fd:// protocol)

```typescript
const file = fs.openSync(uri, fs.OpenMode.READ_ONLY);
playerNapi.setMedia(`fd://${file.fd}`);
```

The file descriptor remains open for the lifetime of playback and is closed when a new file is opened or the app is destroyed.

### Network Playback

Simply pass any supported URL to `setMedia`:
```typescript
playerNapi.setMedia('https://example.com/video.mp4');
playerNapi.setMedia('rtsp://example.com/stream');
```

## Permissions

Declared in `entry/src/main/module.json5`:

| Permission | Purpose |
|---|---|
| `ohos.permission.INTERNET` | Network stream playback |
| `ohos.permission.READ_MEDIA` | Read local media files |
