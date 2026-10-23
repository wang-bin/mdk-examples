# MDK Unity troubleshooting

## Audio plays, but the video surface is black

1. Enable the `Editor` platform for both `MDKUnityPlugin.dll` and `mdk.dll` in the Plugin Importer. The checked-in `.meta` files already do this. Reimport the two assets or restart Tuanjie after replacing a DLL.
2. On Windows, use the MSVC x64 plugin in `Assets/Plugins/Windows/x86_64` and launch once with `-force-d3d11`.
3. Enter an absolute local path or a URL in `MDKPlayer.Media Url`. The player accepts both forms.
4. In the Inspector, check that `VideoTexture` is created and `Renderer` is `D3D11` (renderer value `2`). If the native error says `Unity render backend is unavailable`, the Editor plugin importer is still disabled or the active graphics API is not the one used to build the plugin.
5. For the Built-in pipeline, leave `Render Camera` empty to use `Camera.main`, or assign the camera that should run the plugin event. The command buffer is attached at `AfterEverything` to keep the MDK draw away from Unity UI rendering.
6. D3D11 must create a typed render-target view for Unity's typeless texture resources. Rebuild the native bridge if using an older plugin that passed the texture directly to MDK.
7. The Mesh sample uses a camera-facing Quad and an unlit material. A horizontal Plane viewed along its edge will remain invisible even when its texture contains video.

Both display components enable `Auto Flip Y` to account for the graphics API's texture-coordinate origin. `Flip Y` toggles that result manually for a custom shader or display path.

## OpenGL Core

Launch the Editor with `-force-glcore`; the native renderer value should be `17`.
The bridge preserves GL state around the entire plugin event, including initialization,
texture replacement and player release. This includes the current shader, VAO, buffers,
video texture units, blending, viewport, scissor and camera depth/stencil settings.
Read and draw framebuffer bindings are restored separately. If video readback contains
changing pixels but the Game view is black, rebuild and replace the native bridge.

## D3D12

Launch with `-force-d3d12`; the native renderer value should be `18`.
`RenderTexture.GetNativeTexturePtr()` supplies an `ID3D12Resource*`. It must not be
passed to Unity's `TextureFromNativeTexture`, which accepts a `UnityTextureID` instead.
The bridge creates a typed RTV and passes the view and color format to MDK, while
Unity manages the target's resource state. Modern interfaces record into Unity's
current command list; older supported interfaces submit through `ExecuteCommandList`.
The plugin event declares that it modifies command-list state so Unity restores its
bindings before further drawing.

## Vulkan

Launch with `-force-vulkan`; the native renderer value should be `21`.
Hardware decoding uses `copy=1` (Windows: `D3D11:copy=1`, Android:
`AMediaCodec:copy=1`, Linux: `VAAPI:copy=1`), with FFmpeg software fallback.
Zero-copy hardware frames are not supported by this Vulkan bridge.

Unity may supply a secondary command buffer even outside a render pass. MDK's
offscreen render pass requires a primary command buffer, so the bridge accesses
the texture before `AccessQueue`, flushes Unity's barriers and submits its own
primary command buffer in the queue callback. The callback retains the player
and the accessed texture version until it completes. A fence wait completes each
video draw before upload buffers or old textures can be reused; this adds GPU
synchronization overhead. Unity handles subsequent texture layout transitions.

Use `-force-vulkan-layers` to enable available Vulkan validation layers. Older
Editors can report swapchain acquisition/semaphore errors during Editor startup,
before video playback starts. Compare an Editor-only launch with
`-force-vulkan -force-vulkan-layers -quit` to identify startup errors. This
Editor version can also report invalid depth attachments when the verification
forces `Camera.Render()` in batch mode; use the interactive check for Vulkan.

## Rebuild the native plugin

The plugin must be rebuilt after native source changes:

```powershell
# Run in an initialized MSVC x64 developer terminal, from the repository root.
cmake -S examples/Unity/PluginSource -B build/cl64/unity-plugin -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DMDK_SDK_ROOT=D:\path\to\mdk-sdk `
  -DCMAKE_INSTALL_PREFIX=$PWD/examples/Unity/Assets/Plugins/Windows/x86_64 `
  -DMDK_UNITY_INSTALL_SUBDIR=.
cmake --build build/cl64/unity-plugin --target install
```

## Verify playback inside Unity

For an explicit decoder check, set `MDK_UNITY_VIDEO_DECODER` before launching
the Editor, for example `$env:MDK_UNITY_VIDEO_DECODER='D3D11:copy=0'` in
PowerShell. The bridge selects only that decoder, without software fallback.
This diagnostic override applies to non-Vulkan renderers; Vulkan retains its
required `copy=1` configuration. Enable `MDK_LOG=1` and `MDK_CLOG=1` to capture
the actual hardware output format and interop path. Remove the decoder variable
after testing to restore the normal decoder selection.

Windows/Tuanjie 2022.3.61t9 playback checks with H.264 1920x1080 and
`D3D11:copy=0` passed on D3D11, D3D12 and OpenGL Core: changing video pixels,
correctly oriented RawImage screenshots, texture version acknowledgement and
normal Editor exit. Logs confirmed `d3d11` hardware frames; OpenGL used
`ID3D11Texture2D-WGL` interop and D3D12 consumed native GPU resources.
D3D11 used a GPU copy into a shared texture because decoder and renderer devices
differed. Thus `copy=0` does not imply that every GPU-to-GPU copy is eliminated.
These checks used the D3D11 decoder on all three rendering APIs, not the D3D12
decoder, and do not establish support for every codec, GPU or driver.

With an activated Editor license, stop Play Mode and select **MDK > Verify sample playback**.
The check opens the RawImage sample, assigns the repository test video in memory, enters
Play Mode and reads two video frames. A `MDK_VERIFY PASS` Console message means the
texture contains changing RGB pixels and the display component is bound to that texture.
The frames and an interactive Game screenshot are written to `Temp/MDKVerification`.
Interactive checks also reject a Game screenshot without video colors; visually inspect
the saved screenshot to verify orientation and framing.

For automated D3D11 checks, launch the Editor with `-batchmode -force-d3d11
-projectPath <Unity project> -executeMethod MDK.Unity.Editor.MDKPlaybackVerification.Run
-logFile <log path>` (do not use `-nographics` or `-quit`). The check exits with code 0
on success and code 1 on a playback timeout. `MDK_UNITY_VERIFY_MEDIA`,
`MDK_UNITY_VERIFY_SCENE`, and `MDK_UNITY_VERIFY_OUTPUT` can override the test file,
scene and output directory. Batch mode saves the video textures, without a Game screenshot.

The camera command buffer must restore `CameraTarget` after the plugin event even
when attached to `AfterEverything`: the native context guard restores the video target
that Unity bound before the event. A successful native-only playback test cannot verify
this Unity camera/display step.

## Blurry or duplicated Unity text

If the whole Tuanjie/Unity editor chrome is blurry, this is usually Windows DPI scaling. Open the Tuanjie executable properties, choose **Compatibility > Change high DPI settings**, enable the high-DPI override, choose **Application**, and restart the editor.

If labels are duplicated or show strong ghosting while the Game view is black, this is a stale native plugin that left D3D11 pipeline state bound after the video event. Close the editor before replacing `MDKUnityPlugin.dll`; Unity keeps a loaded native DLL locked. Reopen the project after the replacement so the D3D11 context-state guard is loaded. Do not diagnose this case by changing Canvas scale or Game view zoom.

If only the Game view is blurry, set the Game view scale to `1x` instead of `Stretch`. The sample Canvas uses a fixed pixel scale at the default 1280x720 resolution; avoid scaling a low-resolution Game view to fill a high-resolution monitor.
