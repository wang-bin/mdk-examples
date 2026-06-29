# MDK Compose Multiplatform Example

A media player built with [Compose Multiplatform](https://www.jetbrains.com/lp/compose-multiplatform/)
that integrates the [MDK SDK](https://github.com/wang-bin/mdk-sdk) on:

| Platform | Binding mechanism | Rendering surface |
|---|---|---|
| **iOS** | Kotlin/Native **C interop** → `mdk/c/Player.h` | `UIView` via `UIKitView` |
| **Android** | **JNI** → C++ wrapper (`mdk/Player.h`) | `SurfaceView` via `AndroidView` |
| **Desktop** (Win/macOS/Linux) | **JNA** → pre-built `mdk_shim` C wrapper | AWT `Canvas` via `SwingPanel` |

---

## Project layout

```
composeApp/
├── nativeInterop/cinterop/mdk.def          # C interop definition (iOS)
└── src/
    ├── commonMain/kotlin/…/
    │   ├── App.kt           # Compose UI (shared)
    │   └── MdkPlayer.kt     # expect class + expect VideoSurface composable
    ├── iosMain/kotlin/…/
    │   └── MdkPlayer.ios.kt # actual – Kotlin/Native C interop
    ├── androidMain/
    │   ├── kotlin/…/MdkPlayer.android.kt   # actual – JNI
    │   └── cpp/
    │       ├── CMakeLists.txt
    │       └── MdkPlayerJNI.cpp
    └── desktopMain/
        ├── kotlin/…/
        │   ├── MdkPlayer.desktop.kt        # actual – JNA
        │   └── main.kt
        └── native/
            ├── CMakeLists.txt
            └── mdk_shim.c                  # thin C wrapper for JNA
```

---

## Prerequisites

Download the MDK SDK from:
<https://sourceforge.net/projects/mdk-sdk/files/nightly/>

Different SDK packages are required per target platform.

---

## iOS

### 1. Get the SDK

Download the **iOS** MDK SDK package (e.g. `mdk-sdk-apple.tar.gz`) and extract
it.  The directory should contain `include/` and `lib/mdk.xcframework/`.

### 2. Configure the SDK path

Either:
- Set the environment variable `MDK_SDK_IOS=/path/to/mdk-sdk`, **or**
- Add `mdk.sdk.ios=/path/to/mdk-sdk` to `gradle.properties`, **or**
- Copy / symlink the SDK to `composeApp/libs/mdk-sdk/`

### 3. Link the xcframework

In `composeApp/build.gradle.kts`, add linker options for the iOS binaries:

```kotlin
iosTargets.forEach { iosTarget ->
    iosTarget.binaries.framework {
        val sdkPath = ...           // same path as above
        linkerOpts("-F$sdkPath/lib", "-framework", "mdk")
    }
}
```

### 4. Build

```sh
./gradlew :composeApp:iosDeployIphone   # or open in Xcode via KMP plugin
```

---

## Android

### 1. Get the SDK

Download the **Android** MDK SDK package (e.g. `mdk-sdk-android.tar.gz`) and
extract it to:

```
composeApp/src/androidMain/cpp/mdk-sdk/
```

Or pass a custom path to CMake in `build.gradle.kts`:

```kotlin
externalNativeBuild {
    cmake { arguments("-DMDK_SDK_DIR=/path/to/mdk-sdk") }
}
```

### 2. Build & run

```sh
./gradlew :composeApp:installDebug
```

---

## Desktop (Windows / macOS / Linux)

The Desktop JVM target uses **JNA** to call the `mdk_shim` shared library.
This thin C wrapper (in `src/desktopMain/native/`) abstracts the MDK C API
struct's function pointers behind plain exported C functions.

### 1. Get the SDK

Download the appropriate desktop MDK SDK package for your OS and extract it.

### 2. Build mdk_shim

```sh
cd composeApp/src/desktopMain/native
mkdir build && cd build
cmake .. -DMDK_SDK_DIR=/path/to/mdk-sdk
cmake --build .
```

This produces:
- `libmdk_shim.so`    (Linux)
- `libmdk_shim.dylib` (macOS)
- `mdk_shim.dll`      (Windows)

### 3. Make libraries available at runtime

Place both `libmdk` and `libmdk_shim` on `java.library.path`.  The simplest
approach is to copy them next to the application JAR or pass the directory
when launching:

```sh
./gradlew :composeApp:run -Djava.library.path=/path/to/libs
```

Or configure it in `build.gradle.kts`:

```kotlin
compose.desktop {
    application {
        jvmArgs("-Djava.library.path=/path/to/libs")
    }
}
```

---

## How C interop works (iOS)

Kotlin/Native's [C interop](https://kotlinlang.org/docs/native-c-interop.html)
generates Kotlin bindings from the MDK C header (`mdk/c/Player.h`).  The
binding is configured in `composeApp/nativeInterop/cinterop/mdk.def`.

Key patterns used in `MdkPlayer.ios.kt`:

```kotlin
// Factory – MDK_createPlayer() returns a CPointer<mdkPlayerAPI>
val api: CPointer<mdkPlayerAPI> = MDK_createPlayer()!!

// Call a function pointer field inside the struct
api.pointed.setMedia?.invoke(api, url.cstr.ptr, 0)  // memScoped required for .cstr

// ObjC object → void* bridge (helper defined in mdk.def's code section)
val surfacePtr: COpaquePointer? = mdkUIViewPtr(uiView)
api.pointed.updateNativeSurface?.invoke(api, surfacePtr, width, height, 0)
```

The `.def` file's code section contains Objective-C helper functions
(`mdkUIViewPtr`, `mdkCALayerPtr`) that perform ARC-safe `__bridge void*` casts,
making it straightforward to pass ObjC objects to the MDK C API.

---

## License

Same as the rest of the mdk-examples repository.
