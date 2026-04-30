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
 *   - Windows OpenGL / Linux       → GL texture + FBO render target
 *   - macOS / iOS Metal            → Metal texture render target
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
            else
            {
                // OpenGL on Windows: nativePtr is the GL texture name (GLuint)
                _player.SetOpenGLRenderTarget(nativePtr, IntPtr.Zero, rt.width, rt.height);
            }
#elif UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX || UNITY_IOS
            // Metal: nativePtr is the MTLTexture
            // We need the device pointer — get it via SystemInfo on Metal
            _player.SetMetalRenderTarget(nativePtr, GetMetalDevice(), IntPtr.Zero);
#elif UNITY_ANDROID
            // OpenGL ES: nativePtr is the GL texture name (GLuint)
            _player.SetOpenGLRenderTarget(nativePtr, IntPtr.Zero, rt.width, rt.height);
#elif UNITY_STANDALONE_LINUX
            // OpenGL on Linux
            _player.SetOpenGLRenderTarget(nativePtr, IntPtr.Zero, rt.width, rt.height);
#else
            // Fallback: try OpenGL
            _player.SetOpenGLRenderTarget(nativePtr, IntPtr.Zero, rt.width, rt.height);
#endif
            _player.SetVideoSurfaceSize(rt.width, rt.height);
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
    }
}
