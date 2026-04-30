/*
 * Copyright (c) 2024 WangBin <wbsecg1 at gmail.com>
 * MDK SDK Unity Plugin — MDKPlayerView MonoBehaviour
 *
 * Renders MDK video into a Unity Texture2D that can be applied to any
 * Renderer, UI RawImage, or Skybox.
 *
 * Usage:
 *   1. Attach this component to a GameObject that has a Renderer (MeshRenderer,
 *      SkinnedMeshRenderer) or leave Renderer blank and assign the RawImage
 *      field for UI usage.
 *   2. Set the Url field (Inspector or code) and call Play().
 *
 * Rendering strategy (automatically selected at runtime):
 *   - Windows D3D11 editor/player  → D3D11 texture render target
 *   - Windows D3D12                → D3D12 resource render target (with command-list callback)
 *   - Windows OpenGL / Linux GL    → GL texture + FBO render target
 *   - macOS / iOS Metal            → Metal texture render target
 *   - Vulkan (any platform)        → Vulkan image render target (with image-info / cmd-buf callbacks)
 *   - Android OpenGL ES            → GL texture + FBO render target
 */
using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.UI;
#if UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN
using System.Runtime.InteropServices;
#endif

namespace MDK
{
    [AddComponentMenu("MDK/MDK Player View")]
    public class MDKPlayerView : MonoBehaviour
    {
        // ----------------------------------------------------------------
        // Inspector fields
        // ----------------------------------------------------------------

        [Tooltip("Media URL or local file path to play.")]
        public string Url = "";

        [Tooltip("Auto-play on Start.")]
        public bool AutoPlay = true;

        [Tooltip("Loop playback. -1 = infinite, 0 = no loop.")]
        public int LoopCount = -1;

        [Tooltip("Initial volume (0.0–1.0).")]
        [Range(0f, 1f)]
        public float Volume = 1f;

        [Tooltip("Preferred hardware decoders (comma-separated). Leave empty for auto.")]
        public string Decoders = "";

        [Tooltip("Target RawImage for UI playback. Takes priority over Renderer.")]
        public RawImage TargetRawImage;

        [Tooltip("Target Renderer (e.g. MeshRenderer) for 3D object playback.")]
        public new Renderer renderer;

        // ----------------------------------------------------------------
        // Public events
        // ----------------------------------------------------------------

        public event Action<PlayerState> OnStateChanged;
        public event Action<MediaStatus> OnMediaStatusChanged;

        // ----------------------------------------------------------------
        // Public read-only properties
        // ----------------------------------------------------------------

        public Player Player => _player;

        public long Position => _player?.Position ?? 0;

        public long Duration => _player?.Duration ?? 0;

        public bool IsPlaying => _player?.State == PlayerState.Playing;

        // ----------------------------------------------------------------
        // Private state
        // ----------------------------------------------------------------

        private Player       _player;
        private Texture2D    _texture;
        private RenderTexture _renderTexture;
        private bool         _frameReady;
        private int          _videoWidth;
        private int          _videoHeight;
        private bool         _sizeInitialized;

        // Strong references to native callback delegates to prevent GC collection
        // while the native plugin holds the function pointers.
        private D3D12GetCommandListCallback _d3d12GetCmdListDelegate;
        private VkRenderTargetInfoCallback  _vkGetRTInfoDelegate;
        private VkGetCommandBufferCallback  _vkGetCmdBufDelegate;

        // ----------------------------------------------------------------
        // Unity lifecycle
        // ----------------------------------------------------------------

        private void Awake()
        {
            Player.SetLogLevel(2); // Warning+
            _player = new Player();
            _player.StateChanged        += s => OnStateChanged?.Invoke(s);
            _player.MediaStatusChanged  += s =>
            {
                OnMediaStatusChanged?.Invoke(s);
                if ((s & MediaStatus.Loaded) != 0)
                    // Video dimensions become available once loaded
                    StartCoroutine(WaitForVideoSize());
            };
            _player.FrameReady += () => _frameReady = true;

            if (!string.IsNullOrEmpty(Decoders))
                _player.Decoders = Decoders;

            _player.Volume = Volume;
            _player.Loop   = LoopCount;
        }

        private void Start()
        {
            if (AutoPlay && !string.IsNullOrEmpty(Url))
                Play(Url);
        }

        private void Update()
        {
            if (_frameReady && _sizeInitialized)
            {
                _frameReady = false;
                RenderFrame();
            }
        }

        private void OnDestroy()
        {
            _player?.Dispose();
            _player = null;
            DestroyTextures();
        }

        private void OnApplicationPause(bool paused)
        {
            if (_player == null) return;
            if (paused)
                _player.State = PlayerState.Paused;
            else
                _player.State = PlayerState.Playing;
        }

        // ----------------------------------------------------------------
        // Public API
        // ----------------------------------------------------------------

        /// <summary>Start playing the given URL (or the Inspector URL if null).</summary>
        public void Play(string url = null)
        {
            if (_player == null) return;
            if (url != null) Url = url;
            _player.Url   = Url;
            _player.Prepare();
            _player.State = PlayerState.Playing;
        }

        public void Pause()
        {
            if (_player == null) return;
            if (_player.State == PlayerState.Paused)
                _player.State = PlayerState.Playing;
            else
                _player.State = PlayerState.Paused;
        }

        public void Stop() => _player?.State = PlayerState.Stopped;

        public void Seek(long positionMs) => _player?.Seek(positionMs);

        public float PlaybackRate
        {
            get => _player?.PlaybackRate ?? 1f;
            set { if (_player != null) _player.PlaybackRate = value; }
        }

        // ----------------------------------------------------------------
        // Texture / render management
        // ----------------------------------------------------------------

        private IEnumerator WaitForVideoSize()
        {
            // Poll until video dimensions are known
            while (_player != null && !_player.TryGetVideoSize(out _videoWidth, out _videoHeight))
                yield return null;

            if (_videoWidth <= 0 || _videoHeight <= 0) yield break;

            yield return null; // one frame for GL context to be set up

            InitTexture(_videoWidth, _videoHeight);
        }

        private void InitTexture(int width, int height)
        {
            DestroyTextures();

            _renderTexture = new RenderTexture(width, height, 0, RenderTextureFormat.ARGB32)
            {
                wrapMode   = TextureWrapMode.Clamp,
                filterMode = FilterMode.Bilinear,
            };
            _renderTexture.Create();

            // Tell MDK to render into our RenderTexture using the appropriate graphics API.
            // We issue a render command on the render thread via CommandBuffer.
            SetupRenderAPI(_renderTexture);

            _sizeInitialized = true;

            // Apply to display target
            if (TargetRawImage != null)
                TargetRawImage.texture = _renderTexture;
            else if (renderer != null)
                renderer.material.mainTexture = _renderTexture;
        }

        private void SetupRenderAPI(RenderTexture rt)
        {
            var nativePtr = rt.GetNativeTexturePtr();

#if UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN
            if (SystemInfo.graphicsDeviceType == GraphicsDeviceType.Direct3D11)
            {
                // D3D11: the native pointer IS the ID3D11Texture2D*
                _player.SetD3D11RenderTarget(nativePtr, IntPtr.Zero);
            }
            else if (SystemInfo.graphicsDeviceType == GraphicsDeviceType.Direct3D12)
            {
                // D3D12: nativePtr is ID3D12Resource*.
                // The command queue is obtained via IUnityGraphicsD3D12v5 on the native side.
                // The command-list callback is invoked each frame on the render thread.
                // We pass the native texture pointer as the opaque value so the callback
                // can identify which resource is being rendered.
                _d3d12GetCmdListDelegate = GetD3D12CommandList;
                var cmdQueue = GetD3D12CommandQueue(); // helper — see below
                _player.SetD3D12RenderTarget(nativePtr, cmdQueue, _d3d12GetCmdListDelegate, nativePtr);
            }
            else
            {
                // OpenGL on Windows: nativePtr is the GL texture name (GLuint)
                _player.SetOpenGLRenderTarget(nativePtr, IntPtr.Zero, rt.width, rt.height);
            }
#elif UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX || UNITY_IOS
            if (SystemInfo.graphicsDeviceType == GraphicsDeviceType.Vulkan)
            {
                SetupVulkanRenderAPI(rt, nativePtr);
            }
            else
            {
                // Metal: nativePtr is the MTLTexture
                _player.SetMetalRenderTarget(nativePtr, GetMetalDevice(), IntPtr.Zero);
            }
#elif UNITY_ANDROID
            if (SystemInfo.graphicsDeviceType == GraphicsDeviceType.Vulkan)
                SetupVulkanRenderAPI(rt, nativePtr);
            else
                // OpenGL ES: nativePtr is the GL texture name (GLuint)
                _player.SetOpenGLRenderTarget(nativePtr, IntPtr.Zero, rt.width, rt.height);
#elif UNITY_STANDALONE_LINUX
            if (SystemInfo.graphicsDeviceType == GraphicsDeviceType.Vulkan)
                SetupVulkanRenderAPI(rt, nativePtr);
            else
                _player.SetOpenGLRenderTarget(nativePtr, IntPtr.Zero, rt.width, rt.height);
#else
            // Fallback: try OpenGL
            _player.SetOpenGLRenderTarget(nativePtr, IntPtr.Zero, rt.width, rt.height);
#endif
            _player.SetVideoSurfaceSize(rt.width, rt.height);
        }

        // Configures a Vulkan render target using the VkImage returned by GetNativeTexturePtr().
        private void SetupVulkanRenderAPI(RenderTexture rt, IntPtr vkImage)
        {
            var device    = GetVulkanDevice();
            var phyDevice = GetVulkanPhysicalDevice();

            // Capture dimensions for the renderTargetInfo callback.
            int w = rt.width, h = rt.height;

            _vkGetRTInfoDelegate = (IntPtr _, out int ow, out int oh, out int fmt, out int layout) =>
            {
                ow  = w;
                oh  = h;
                fmt    = 37;  // VK_FORMAT_R8G8B8A8_UNORM
                layout = 5;   // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                return 1;
            };
            _vkGetCmdBufDelegate = (_) => GetVulkanCommandBuffer();

            _player.SetVulkanRenderTarget(device, phyDevice, vkImage,
                _vkGetRTInfoDelegate, _vkGetCmdBufDelegate);
        }

        private void RenderFrame()
        {
            if (_player == null || _renderTexture == null) return;

            // Issue the renderVideo call on the render thread so the correct
            // graphics context is current when mdk renders.
            var cb = new CommandBuffer { name = "MDK RenderVideo" };
            cb.IssuePluginCustomTextureUpdateV2(IntPtr.Zero, _renderTexture, 0);
            // For non-texture-update APIs, we use a helper MonoBehaviour render coroutine:
            // actually just call it directly here — Unity's render thread is synchronous
            // within a CommandBuffer on most platforms when using GetTemporaryRT.
            // The simplest reliable approach: call from GL.IssuePluginEvent.
            // Here we use a direct call wrapped in a GL callback via IssuePluginEvent.
            Graphics.ExecuteCommandBuffer(cb);
            cb.Dispose();

            // Simpler direct call if the above texture-update path is not available:
            _player.RenderVideo();
        }

        private void DestroyTextures()
        {
            if (_renderTexture != null)
            {
                _player?.SetVideoSurfaceSize(-1, -1); // release renderer resources
                _renderTexture.Release();
                Destroy(_renderTexture);
                _renderTexture = null;
            }
            if (_texture != null)
            {
                Destroy(_texture);
                _texture = null;
            }
            _sizeInitialized = false;
        }

        // ----------------------------------------------------------------
        // Platform helpers
        // ----------------------------------------------------------------

#if UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX || UNITY_IOS
        [System.Runtime.InteropServices.DllImport("__Internal")]
        private static extern IntPtr MDKUnity_GetMetalDevice();

        private static IntPtr GetMetalDevice()
        {
            try { return MDKUnity_GetMetalDevice(); }
            catch { return IntPtr.Zero; }
        }
#else
        private static IntPtr GetMetalDevice() => IntPtr.Zero;
#endif

        // ----------------------------------------------------------------
        // D3D12 platform helpers (Windows)
        // The actual command-queue and command-list pointers must come from
        // the IUnityGraphicsD3D12v5 interface which is available only in a
        // native C++ Unity plugin (IUnityInterfaces).  The stubs below show
        // the expected signatures; replace them with P/Invoke calls into a
        // helper native plugin that calls IUnityGraphicsD3D12v5 on your behalf.
        // ----------------------------------------------------------------

#if UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN
        /// <summary>
        /// Returns the ID3D12CommandQueue* used by Unity's D3D12 renderer.
        /// Replace this stub with an actual P/Invoke into a helper native plugin
        /// that calls IUnityGraphicsD3D12v5::GetCommandQueue().
        /// </summary>
        private static IntPtr GetD3D12CommandQueue()
        {
            // TODO: return the ID3D12CommandQueue* from IUnityGraphicsD3D12v5.
            return IntPtr.Zero;
        }

        /// <summary>
        /// Returns the current ID3D12GraphicsCommandList* for the ongoing frame.
        /// Called by MDK on the render thread each time it needs to record commands.
        /// Replace with a call into a helper native plugin that returns
        /// IUnityGraphicsD3D12v5::GetFrameCommandList(frameIndex).
        /// <param name="opaque">The ID3D12Resource* passed as opaque to SetD3D12RenderTarget.</param>
        /// </summary>
        private static IntPtr GetD3D12CommandList(IntPtr opaque)
        {
            // TODO: return the ID3D12GraphicsCommandList* for the current frame.
            return IntPtr.Zero;
        }
#else
        private static IntPtr GetD3D12CommandQueue()  => IntPtr.Zero;
        private static IntPtr GetD3D12CommandList(IntPtr _) => IntPtr.Zero;
#endif

        // ----------------------------------------------------------------
        // Vulkan platform helpers
        // VkDevice and VkPhysicalDevice are accessible via IUnityGraphicsVulkanV2
        // (native plugin interface).  Provide them through a small helper native
        // plugin that calls IUnityGraphicsVulkanV2::Instance() on your behalf.
        // ----------------------------------------------------------------

        private static IntPtr GetVulkanDevice()
        {
            // TODO: return VkDevice from IUnityGraphicsVulkanV2::Instance().device
            return IntPtr.Zero;
        }

        private static IntPtr GetVulkanPhysicalDevice()
        {
            // TODO: return VkPhysicalDevice from IUnityGraphicsVulkanV2::Instance().physicalDevice
            return IntPtr.Zero;
        }

        /// <summary>
        /// Returns the VkCommandBuffer currently being recorded by Unity's Vulkan renderer.
        /// Called by MDK on the render thread.
        /// Replace with a call into a helper native plugin that reads
        /// IUnityGraphicsVulkanV2::CommandRecordingState().commandBuffer.
        /// </summary>
        private static IntPtr GetVulkanCommandBuffer()
        {
            // TODO: return VkCommandBuffer from IUnityGraphicsVulkanV2::CommandRecordingState().
            return IntPtr.Zero;
        }
    }
}
