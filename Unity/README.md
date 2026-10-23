# MDK Unity multi-backend example

MDK video playback into a Unity-owned `RenderTexture`, with RawImage and Mesh samples.

Start with the [project guide (中文)](UNITY_PLUGIN_GUIDE.md) for build, usage and architecture. Use [troubleshooting](TROUBLESHOOTING.md) for audio-only playback, display corruption and backend diagnostics.

## Scope and verification

Unity/Tuanjie 2022.3 is the baseline. The main sample uses Built-in. The texture bridge has an SRP scheduling path, but the sample shaders are not universal URP/HDRP materials, and not every pipeline/platform combination has been tested.

| Platform | Backend code coverage | Verification |
| --- | --- | --- |
| Windows | D3D11, D3D12, Vulkan, OpenGL Core | D3D11/D3D12/OpenGL SDR checks passed. Vulkan passed earlier checks but the latest regression crashed in Editor startup before playback; it is not currently a passing regression. |
| macOS | Metal, OpenGL Core where available | Not validated on hardware in this work. |
| Linux | Vulkan, OpenGL Core | Not validated on hardware in this work. |
| Android | Vulkan, OpenGL ES 3 | Not validated on devices in this work. |
| iOS | Metal | Not validated on devices in this work. |

Vulkan hardware decoding currently requires `copy=1`. Its compatibility path submits a plugin primary command buffer through Unity's `AccessQueue` and waits for completion; this has synchronization overhead.

## First playback

1. Build the native plugin and deploy matching MDK/FFmpeg runtime libraries; see the guide's Windows `build/cl64` instructions.
2. Open `examples/Unity` in the Editor.
3. Open `Assets/MDKUnity/Samples/Scenes/MDKPlayer.unity`, assign a local path or URL, enter Play Mode and inspect the **Game** view.
4. Use `MDKMeshPlayer.unity` for the Quad/MaterialPropertyBlock sample.

Keep source and Unity meta files together. Do not commit built plugins or SDK binaries.

## Dependencies

The main manifest retains uGUI, the test framework and the engine modules used by playback verification. Ads, analytics, purchasing, 2D, XR and IDE template packages are not required.

`Packages/packages-lock.json` is generated locally and ignored: Unity and Tuanjie resolve against their selected registries. This avoids distributing a Tuanjie-specific registry lock, but transitive resolution is not locked across machines. Archive the resolved package list with release/test reports; clean-editor resolution must be validated separately.

The reduced manifest passed a clean, no-graphics Tuanjie 2022.3.61t9 import and C# compilation check. Official Unity resolution and GPU/Player behavior were not revalidated by this dependency cleanup.

## Optional HDR validation

D3D11/D3D12 have optional FP16 scRGB texture output. HDR texture values have been tested; this does not imply complete HDR screen validation.

The separate `Validation/HDRPProject` and HDR tools are **experimental, opt-in validation assets**, not prerequisites for the Built-in sample. HDRP 14.0.12 scRGB output was observed active; HDR10 runtime display acceptance and the latest diagnostic Player rebuild remain incomplete. See the [HDR usage reference](HDR_OUTPUT.md).

## Maintainer references

- [MDK integration design proposals](docs/design/MDK_UNITY_INTEGRATION_DESIGN.md)
- [Vulkan design proposals](docs/design/MDK_VULKAN_IMPROVEMENTS.md)

Design proposals describe future work, not shipped APIs or verified capabilities.
