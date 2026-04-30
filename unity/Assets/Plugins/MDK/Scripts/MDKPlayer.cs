/*
 * Copyright (c) 2024 WangBin <wbsecg1 at gmail.com>
 * MDK SDK Unity Plugin — C# P/Invoke bindings
 *
 * Mirrors the C API exposed by the native mdk-unity plugin (MDKPlayer.h).
 * Works on Windows (D3D11/D3D12/OpenGL), macOS (Metal/OpenGL), Linux (OpenGL/Vulkan),
 * iOS (Metal), and Android (OpenGL ES).
 */
using System;
using System.Runtime.InteropServices;

namespace MDK
{
    // -----------------------------------------------------------------------
    // Enums (must match MDKPlayer.h)
    // -----------------------------------------------------------------------

    public enum PlayerState
    {
        Stopped = 0,
        Playing = 1,
        Paused  = 2,
    }

    [Flags]
    public enum MediaStatus
    {
        NoMedia   = 0,
        Unloaded  = 1,
        Loading   = 2,
        Loaded    = 4,
        Prepared  = 8,
        Stalled   = 16,
        Buffering = 32,
        Buffered  = 64,
        End       = 128,
        Seeking   = 256,
        Invalid   = unchecked((int)0x80000000),
    }

    // -----------------------------------------------------------------------
    // Delegate types for native callbacks
    // -----------------------------------------------------------------------

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void StateChangedCallback(PlayerState state, IntPtr userdata);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void RenderCallback(IntPtr userdata);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void MediaStatusChangedCallback(MediaStatus status, IntPtr userdata);

    /// <summary>
    /// Callback invoked by MDKPlayer_setD3D12RenderTarget to obtain the current
    /// ID3D12GraphicsCommandList* that MDK should record into.
    /// Return the command list pointer (as IntPtr), or IntPtr.Zero if unavailable.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate IntPtr D3D12GetCommandListCallback(IntPtr opaque);

    /// <summary>
    /// Callback invoked by MDKPlayer_setVulkanRenderTarget to obtain render-target info.
    /// Fill w, h, vkFormat (VkFormat as int), vkImageLayout (VkImageLayout as int).
    /// Return 1 on success, 0 if unavailable.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int VkRenderTargetInfoCallback(IntPtr opaque,
                                                   out int w, out int h,
                                                   out int vkFormat, out int vkImageLayout);

    /// <summary>
    /// Callback invoked by MDKPlayer_setVulkanRenderTarget to obtain the current
    /// VkCommandBuffer (as IntPtr) that MDK should record into.
    /// Return IntPtr.Zero if unavailable.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate IntPtr VkGetCommandBufferCallback(IntPtr opaque);

    // -----------------------------------------------------------------------
    // Raw P/Invoke declarations
    // -----------------------------------------------------------------------

    internal static class NativeMethods
    {
#if UNITY_IOS && !UNITY_EDITOR
        private const string LibName = "__Internal";
#else
        private const string LibName = "mdk-unity";
#endif

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr MDKPlayer_create();

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_destroy(IntPtr player);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setMedia(IntPtr player, [MarshalAs(UnmanagedType.LPUTF8Str)] string url);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr MDKPlayer_getMedia(IntPtr player);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setState(IntPtr player, PlayerState state);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern PlayerState MDKPlayer_getState(IntPtr player);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_seek(IntPtr player, long positionMs);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern long MDKPlayer_getPosition(IntPtr player);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern long MDKPlayer_getDuration(IntPtr player);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setPlaybackRate(IntPtr player, float rate);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern float MDKPlayer_getPlaybackRate(IntPtr player);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setVolume(IntPtr player, float volume);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setLoop(IntPtr player, int count);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setVideoSurfaceSize(IntPtr player, int width, int height);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern double MDKPlayer_renderVideo(IntPtr player);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setOpenGLRenderTarget(IntPtr player, IntPtr textureId, IntPtr fboId, int width, int height);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setD3D11RenderTarget(IntPtr player, IntPtr texture, IntPtr context);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setD3D12RenderTarget(IntPtr player,
                                                                  IntPtr rt,
                                                                  IntPtr cmdQueue,
                                                                  D3D12GetCommandListCallback getCmdList,
                                                                  IntPtr opaque);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setMetalRenderTarget(IntPtr player, IntPtr texture, IntPtr device, IntPtr cmdQueue);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setVulkanRenderTarget(IntPtr player,
                                                                   IntPtr device,
                                                                   IntPtr phy_device,
                                                                   IntPtr rt,
                                                                   VkRenderTargetInfoCallback getRTInfo,
                                                                   VkGetCommandBufferCallback getCmdBuf,
                                                                   IntPtr opaque);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setStateChangedCallback(IntPtr player, StateChangedCallback cb, IntPtr userdata);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setRenderCallback(IntPtr player, RenderCallback cb, IntPtr userdata);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setMediaStatusChangedCallback(IntPtr player, MediaStatusChangedCallback cb, IntPtr userdata);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool MDKPlayer_getVideoSize(IntPtr player, out int width, out int height);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_setDecoders(IntPtr player, [MarshalAs(UnmanagedType.LPUTF8Str)] string decoders);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDKPlayer_prepare(IntPtr player, long positionMs);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int MDKPlayer_getMediaStatus(IntPtr player);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void MDK_setLogLevel(int level);
    }

    // -----------------------------------------------------------------------
    // High-level managed wrapper
    // -----------------------------------------------------------------------

    /// <summary>
    /// Managed wrapper around the native MDK Player.
    /// Create one instance per video stream; dispose when done.
    /// </summary>
    public sealed class Player : IDisposable
    {
        private IntPtr _handle;
        private bool   _disposed;

        // Keep delegates alive to prevent GC from collecting them while
        // the native side holds function pointers.
        private StateChangedCallback       _stateChangedDelegate;
        private RenderCallback             _renderDelegate;
        private MediaStatusChangedCallback _mediaStatusDelegate;

        public event Action<PlayerState>  StateChanged;
        public event Action               FrameReady;
        public event Action<MediaStatus>  MediaStatusChanged;

        public Player()
        {
            _handle = NativeMethods.MDKPlayer_create();
            if (_handle == IntPtr.Zero)
                throw new InvalidOperationException("Failed to create MDK player.");

            // Wire up internal delegates — keep field references to prevent GC
            _stateChangedDelegate = (s, _) => StateChanged?.Invoke(s);
            _renderDelegate       = (_)    => FrameReady?.Invoke();
            _mediaStatusDelegate  = (s, _) => MediaStatusChanged?.Invoke(s);

            NativeMethods.MDKPlayer_setStateChangedCallback(_handle, _stateChangedDelegate, IntPtr.Zero);
            NativeMethods.MDKPlayer_setRenderCallback(_handle, _renderDelegate, IntPtr.Zero);
            NativeMethods.MDKPlayer_setMediaStatusChangedCallback(_handle, _mediaStatusDelegate, IntPtr.Zero);
        }

        ~Player() => Dispose(false);

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (_disposed) return;
            if (_handle != IntPtr.Zero)
            {
                // Release GL / D3D resources before destroying
                NativeMethods.MDKPlayer_setVideoSurfaceSize(_handle, -1, -1);
                NativeMethods.MDKPlayer_destroy(_handle);
                _handle = IntPtr.Zero;
            }
            _disposed = true;
        }

        // ----------------------------------------------------------------
        // Media / Playback
        // ----------------------------------------------------------------

        /// <summary>Set the media URL or local file path to play.</summary>
        public string Url
        {
            get
            {
                var ptr = NativeMethods.MDKPlayer_getMedia(_handle);
                return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUTF8(ptr);
            }
            set => NativeMethods.MDKPlayer_setMedia(_handle, value ?? string.Empty);
        }

        public PlayerState State
        {
            get => NativeMethods.MDKPlayer_getState(_handle);
            set => NativeMethods.MDKPlayer_setState(_handle, value);
        }

        /// <summary>Current playback position in milliseconds.</summary>
        public long Position => NativeMethods.MDKPlayer_getPosition(_handle);

        /// <summary>Total media duration in milliseconds (0 if unknown).</summary>
        public long Duration => NativeMethods.MDKPlayer_getDuration(_handle);

        /// <summary>Seek to the given position (milliseconds).</summary>
        public void Seek(long positionMs) =>
            NativeMethods.MDKPlayer_seek(_handle, positionMs);

        /// <summary>Playback speed multiplier (1.0 = normal).</summary>
        public float PlaybackRate
        {
            get => NativeMethods.MDKPlayer_getPlaybackRate(_handle);
            set => NativeMethods.MDKPlayer_setPlaybackRate(_handle, value);
        }

        /// <summary>Master audio volume (0.0–1.0).</summary>
        public float Volume
        {
            set => NativeMethods.MDKPlayer_setVolume(_handle, value);
        }

        /// <summary>Loop count. -1 = loop forever, 0 = no loop.</summary>
        public int Loop
        {
            set => NativeMethods.MDKPlayer_setLoop(_handle, value);
        }

        /// <summary>
        /// Preferred hardware decoder names, comma-separated.
        /// Examples: "VideoToolbox,FFmpeg"  "D3D11,DXVA,FFmpeg"
        /// </summary>
        public string Decoders
        {
            set => NativeMethods.MDKPlayer_setDecoders(_handle, value);
        }

        /// <summary>
        /// Start preparing the media at the given position.
        /// After calling this, call <c>State = PlayerState.Playing</c> to begin playback.
        /// </summary>
        public void Prepare(long positionMs = 0) =>
            NativeMethods.MDKPlayer_prepare(_handle, positionMs);

        public MediaStatus MediaStatus =>
            (MediaStatus)NativeMethods.MDKPlayer_getMediaStatus(_handle);

        /// <summary>Returns the native video dimensions, or false if unavailable.</summary>
        public bool TryGetVideoSize(out int width, out int height) =>
            NativeMethods.MDKPlayer_getVideoSize(_handle, out width, out height);

        // ----------------------------------------------------------------
        // Render surface
        // ----------------------------------------------------------------

        /// <summary>
        /// Notify the player that the render surface has been resized.
        /// Call from the render thread. Use -1,-1 to release GL resources
        /// before destroying the GL context.
        /// </summary>
        public void SetVideoSurfaceSize(int width, int height) =>
            NativeMethods.MDKPlayer_setVideoSurfaceSize(_handle, width, height);

        /// <summary>
        /// Render the current video frame.
        /// Must be called from the render thread (GL context current, or D3D/Metal state set).
        /// Returns the rendered frame timestamp in ms, negative if no new frame.
        /// </summary>
        public double RenderVideo() =>
            NativeMethods.MDKPlayer_renderVideo(_handle);

        /// <summary>
        /// Set an OpenGL texture + FBO as the render target.
        /// Call from the render thread with the GL context current.
        /// Pass fboId = 0 to let the plugin manage the FBO internally.
        /// </summary>
        public void SetOpenGLRenderTarget(IntPtr textureId, IntPtr fboId, int width, int height) =>
            NativeMethods.MDKPlayer_setOpenGLRenderTarget(_handle, textureId, fboId, width, height);

        /// <summary>
        /// Set a D3D11 texture as the render target (Windows only).
        /// Pass context = IntPtr.Zero to use the texture device's immediate context.
        /// </summary>
        public void SetD3D11RenderTarget(IntPtr texture, IntPtr context = default) =>
            NativeMethods.MDKPlayer_setD3D11RenderTarget(_handle, texture, context);

        /// <summary>
        /// Set a D3D12 resource as the render target (Windows only).
        /// <para><paramref name="rt"/>: ID3D12Resource* of the render-target texture.</para>
        /// <para><paramref name="cmdQueue"/>: ID3D12CommandQueue* used for GPU work.</para>
        /// <para><paramref name="getCmdList"/>: callback returning the currently open
        ///   ID3D12GraphicsCommandList* (as IntPtr) for MDK to record into.
        ///   Keep a strong reference to the delegate to prevent GC collection.</para>
        /// <para><paramref name="opaque"/>: forwarded verbatim to <paramref name="getCmdList"/>.</para>
        /// </summary>
        public void SetD3D12RenderTarget(IntPtr rt,
                                         IntPtr cmdQueue,
                                         D3D12GetCommandListCallback getCmdList,
                                         IntPtr opaque = default) =>
            NativeMethods.MDKPlayer_setD3D12RenderTarget(_handle, rt, cmdQueue, getCmdList, opaque);

        /// <summary>
        /// Set a Metal texture as the render target (Apple platforms only).
        /// Pass cmdQueue = IntPtr.Zero to use the default command queue.
        /// </summary>
        public void SetMetalRenderTarget(IntPtr texture, IntPtr device, IntPtr cmdQueue = default) =>
            NativeMethods.MDKPlayer_setMetalRenderTarget(_handle, texture, device, cmdQueue);

        /// <summary>
        /// Set a Vulkan image as the render target (any platform with Vulkan support).
        /// <para><paramref name="device"/>: VkDevice handle.</para>
        /// <para><paramref name="phyDevice"/>: VkPhysicalDevice handle.</para>
        /// <para><paramref name="rt"/>: VkImage handle of the render-target image.
        ///   In Unity, obtain via <c>RenderTexture.GetNativeTexturePtr()</c> when
        ///   <c>SystemInfo.graphicsDeviceType == GraphicsDeviceType.Vulkan</c>.</para>
        /// <para><paramref name="getRTInfo"/>: callback that fills in image dimensions
        ///   and VkFormat/VkImageLayout for the render target.
        ///   Keep a strong reference to the delegate to prevent GC collection.</para>
        /// <para><paramref name="getCmdBuf"/>: callback that returns the currently recording
        ///   VkCommandBuffer (as IntPtr). Keep a strong delegate reference.</para>
        /// <para><paramref name="opaque"/>: forwarded verbatim to both callbacks.</para>
        /// </summary>
        public void SetVulkanRenderTarget(IntPtr device,
                                          IntPtr phyDevice,
                                          IntPtr rt,
                                          VkRenderTargetInfoCallback getRTInfo,
                                          VkGetCommandBufferCallback getCmdBuf,
                                          IntPtr opaque = default) =>
            NativeMethods.MDKPlayer_setVulkanRenderTarget(
                _handle, device, phyDevice, rt, getRTInfo, getCmdBuf, opaque);

        // ----------------------------------------------------------------
        // Global settings
        // ----------------------------------------------------------------

        /// <summary>
        /// Set global MDK log level.
        /// 0 = Off, 1 = Error, 2 = Warning, 3 = Info, 4 = Debug, 5 = All.
        /// </summary>
        public static void SetLogLevel(int level) =>
            NativeMethods.MDK_setLogLevel(level);
    }
}
