/*
 * Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Experimental.Rendering;

namespace MDK.Unity
{
    public enum MDKOutputMode { SDR = 0, HDRLinear = 1 }

    [DisallowMultipleComponent]
    public sealed class MDKPlayer : MonoBehaviour
    {
        [SerializeField] private string mediaUrl = "";
        [SerializeField] private bool playOnEnable = true;
        [SerializeField] private int fallbackWidth = 1280;
        [SerializeField] private int fallbackHeight = 720;
        [SerializeField] private int loopCount;
        [SerializeField] private Camera renderCamera;
        [SerializeField] private MDKOutputMode outputMode;
        private MDKOutputMode textureOutputMode;

        private ulong nativeHandle;
        private IntPtr renderEvent;
        private CommandBuffer commandBuffer;
        private Camera attachedCamera;
        private bool cameraCommandBufferAttached;
        private IntPtr boundNativeTexture;
        private ulong targetVersion;
        private float nextInfoPoll;
        private MDKNative.PlayerInfo nativeInfo;
        private readonly List<PendingTexture> pendingTextures = new List<PendingTexture>();

        private readonly struct PendingTexture
        {
            public readonly RenderTexture texture;
            public readonly ulong version;

            public PendingTexture(RenderTexture texture, ulong version)
            {
                this.texture = texture;
                this.version = version;
            }
        }

        public event Action<RenderTexture> TextureChanged;
        public event Action<MDKNative.PlayerInfo> InfoChanged;

        public string MediaUrl { get => mediaUrl; set => mediaUrl = value ?? string.Empty; }
        public MDKOutputMode OutputMode {
            get => outputMode;
            set {
                outputMode = value;
                if (nativeHandle != 0)
                    EnsureRenderTexture(VideoTexture != null ? VideoTexture.width : fallbackWidth,
                        VideoTexture != null ? VideoTexture.height : fallbackHeight);
            }
        }
        public RenderTexture VideoTexture { get; private set; }
        public int VideoWidth => nativeInfo.width;
        public int VideoHeight => nativeInfo.height;
        public float PixelAspect => nativeInfo.pixelAspect <= 0 ? 1.0f : nativeInfo.pixelAspect;
        public int Rotation => ((nativeInfo.rotation % 360) + 360) % 360;
        public float DisplayAspect {
            get {
                var width = Mathf.Max(1, nativeInfo.width);
                var height = Mathf.Max(1, nativeInfo.height);
                if (Rotation == 90 || Rotation == 270) {
                    var swap = width;
                    width = height;
                    height = swap;
                }
                return width * PixelAspect / height;
            }
        }
        public bool IsLoaded => (nativeInfo.mediaStatus & MDKNative.MediaLoaded) != 0;
        public bool HasError => nativeInfo.error != 0 || (nativeInfo.mediaStatus & MDKNative.MediaInvalid) != 0;
        public int State => nativeInfo.state;
        public int Renderer => nativeInfo.renderer;
        public ulong TargetVersion => targetVersion;
        public ulong AppliedTextureVersion => nativeInfo.appliedVersion;
        public long Position => nativeInfo.position;
        public long Duration => nativeInfo.duration;
        public string ErrorMessage {
            get {
                if (nativeHandle == 0)
                    return string.Empty;
                var buffer = Marshal.AllocHGlobal(512);
                try {
                    MDKNative.MDKUnityPlayerCopyError(nativeHandle, buffer, 512);
                    return Marshal.PtrToStringAnsi(buffer) ?? string.Empty;
                } finally {
                    Marshal.FreeHGlobal(buffer);
                }
            }
        }

        private void Awake()
        {
            nativeHandle = MDKNative.MDKUnityPlayerCreate();
            renderEvent = MDKNative.MDKUnityGetRenderEventFunc();
            commandBuffer = new CommandBuffer { name = "MDK Unity Video" };
            TryAttachBuiltInCommandBuffer();
            EnsureRenderTexture(fallbackWidth, fallbackHeight);
        }

        private void OnEnable()
        {
            if (nativeHandle == 0)
                return;
            TryAttachBuiltInCommandBuffer();
            if (!string.IsNullOrWhiteSpace(mediaUrl))
                Open(mediaUrl, playOnEnable);
        }

        private void Update()
        {
            if (nativeHandle == 0)
                return;
            if (VideoTexture != null && textureOutputMode != outputMode)
                EnsureRenderTexture(VideoTexture.width, VideoTexture.height);
            if (Time.unscaledTime >= nextInfoPoll) {
                nextInfoPoll = Time.unscaledTime + 0.1f;
                PollInfo();
                ReclaimTextures(nativeInfo.appliedVersion);
            }
            RefreshNativeTextureBinding();
            IssueRenderEvent();
        }

        private void OnDisable()
        {
            DetachBuiltInCommandBuffer();
            if (nativeHandle != 0)
                Pause();
        }

        private void OnDestroy()
        {
            DetachBuiltInCommandBuffer();
            var handle = nativeHandle;
            nativeHandle = 0;
            if (handle != 0)
                MDKNative.MDKUnityPlayerDestroy(handle);
            if (handle != 0 && renderEvent != IntPtr.Zero && commandBuffer != null) {
                commandBuffer.Clear();
                commandBuffer.IssuePluginEventAndData(renderEvent, MDKNative.ReleaseEvent, new IntPtr(unchecked((long)handle)));
                Graphics.ExecuteCommandBuffer(commandBuffer);
            }
            if (VideoTexture != null)
                Destroy(VideoTexture);
            for (var i = 0; i < pendingTextures.Count; ++i)
                Destroy(pendingTextures[i].texture);
            pendingTextures.Clear();
            if (commandBuffer != null) {
                commandBuffer.Release();
                commandBuffer = null;
            }
        }

        public void Open(string url, bool play = true)
        {
            mediaUrl = url ?? string.Empty;
            if (nativeHandle == 0)
                return;
            MDKNative.MDKUnityPlayerSetMedia(nativeHandle, mediaUrl);
            MDKNative.MDKUnityPlayerSetLoop(nativeHandle, loopCount);
            MDKNative.MDKUnityPlayerSetState(nativeHandle, play ? MDKNative.StatePlaying : MDKNative.StatePaused);
        }

        public void Play()
        {
            if (nativeHandle != 0)
                MDKNative.MDKUnityPlayerSetState(nativeHandle, MDKNative.StatePlaying);
        }

        public void Pause()
        {
            if (nativeHandle != 0)
                MDKNative.MDKUnityPlayerSetState(nativeHandle, MDKNative.StatePaused);
        }

        public void Stop()
        {
            if (nativeHandle != 0)
                MDKNative.MDKUnityPlayerSetState(nativeHandle, MDKNative.StateStopped);
        }

        public void SetLoop(int count)
        {
            loopCount = Mathf.Max(0, count);
            if (nativeHandle != 0)
                MDKNative.MDKUnityPlayerSetLoop(nativeHandle, loopCount);
        }

        private void PollInfo()
        {
            MDKNative.MDKUnityPlayerGetInfo(nativeHandle, out var info);
            nativeInfo = info;
            if (info.width > 0 && info.height > 0 &&
                (VideoTexture == null || VideoTexture.width != info.width || VideoTexture.height != info.height))
                EnsureRenderTexture(info.width, info.height);
            InfoChanged?.Invoke(info);
        }

        private void EnsureRenderTexture(int width, int height)
        {
            width = Mathf.Max(1, width);
            height = Mathf.Max(1, height);
            if (VideoTexture != null && VideoTexture.width == width && VideoTexture.height == height && textureOutputMode == outputMode)
                return;

            if (outputMode == MDKOutputMode.HDRLinear &&
                ((SystemInfo.graphicsDeviceType != GraphicsDeviceType.Direct3D11 &&
                  SystemInfo.graphicsDeviceType != GraphicsDeviceType.Direct3D12) ||
                 !SystemInfo.IsFormatSupported(GraphicsFormat.R16G16B16A16_SFloat, FormatUsage.Render) ||
                 !SystemInfo.IsFormatSupported(GraphicsFormat.R16G16B16A16_SFloat, FormatUsage.Sample)))
                throw new NotSupportedException("HDRLinear requires D3D11/D3D12 with renderable, sampleable RGBA16F support.");

            var oldTexture = VideoTexture;
            var descriptor = new RenderTextureDescriptor(width, height,
                outputMode == MDKOutputMode.HDRLinear ? RenderTextureFormat.ARGBHalf : RenderTextureFormat.ARGB32, 0) {
                msaaSamples = 1,
                useMipMap = false,
                autoGenerateMips = false,
                sRGB = outputMode == MDKOutputMode.SDR && QualitySettings.activeColorSpace == ColorSpace.Linear,
            };
            var newTexture = new RenderTexture(descriptor) { name = "MDK Video" };
            if (!newTexture.Create()) {
                Destroy(newTexture);
                throw new InvalidOperationException("Could not create MDK video render texture.");
            }
            VideoTexture = newTexture;
            textureOutputMode = outputMode;
            targetVersion++;
            boundNativeTexture = VideoTexture.GetNativeTexturePtr();
            BindRenderTarget();
            TextureChanged?.Invoke(VideoTexture);
            if (oldTexture != null)
                pendingTextures.Add(new PendingTexture(oldTexture, targetVersion));
        }

        private void RefreshNativeTextureBinding()
        {
            if (nativeHandle == 0 || VideoTexture == null || !VideoTexture.IsCreated())
                return;
            var nativeTexture = VideoTexture.GetNativeTexturePtr();
            if (nativeTexture == IntPtr.Zero || nativeTexture == boundNativeTexture)
                return;
            boundNativeTexture = nativeTexture;
            targetVersion++;
            BindRenderTarget();
        }

        private void BindRenderTarget()
        {
            var target = new MDKNative.RenderTargetV2 {
                structSize = (uint)Marshal.SizeOf<MDKNative.RenderTargetV2>(),
                outputMode = textureOutputMode,
                nativeTexture = boundNativeTexture,
                width = VideoTexture.width,
                height = VideoTexture.height,
                version = targetVersion,
            };
            if (MDKNative.MDKUnityPlayerSetRenderTargetV2(nativeHandle, ref target) == 0)
                throw new InvalidOperationException("MDK rejected the render target: " + ErrorMessage);
        }

        private void ReclaimTextures(ulong appliedVersion)
        {
            for (var i = pendingTextures.Count - 1; i >= 0; --i) {
                var pending = pendingTextures[i];
                if (pending.version > appliedVersion)
                    continue;
                if (pending.texture != null)
                    Destroy(pending.texture);
                pendingTextures.RemoveAt(i);
            }
        }

        private void IssueRenderEvent()
        {
            if (renderEvent == IntPtr.Zero || VideoTexture == null || !VideoTexture.IsCreated())
                return;
            TryAttachBuiltInCommandBuffer();
            commandBuffer.Clear();
            commandBuffer.SetRenderTarget(VideoTexture);
            commandBuffer.IssuePluginEventAndData(renderEvent, MDKNative.RenderEvent,
                new IntPtr(unchecked((long)nativeHandle)));
            // Restore the camera target on both paths. The native D3D11 state
            // guard restores the video target selected above, not the camera target.
            commandBuffer.SetRenderTarget(BuiltinRenderTextureType.CameraTarget);
            if (cameraCommandBufferAttached)
                return;

            // Graphics.ExecuteCommandBuffer() is issued outside a camera pass
            // for SRP and for projects without a tagged camera.  Restore the
            // camera target after the plugin event so MDK cannot leave the
            // Unity UI/backbuffer bound to the video RenderTexture.
            Graphics.ExecuteCommandBuffer(commandBuffer);
            Graphics.SetRenderTarget(null);
        }

        private void TryAttachBuiltInCommandBuffer()
        {
            if (cameraCommandBufferAttached || commandBuffer == null ||
                GraphicsSettings.currentRenderPipeline != null)
                return;

            var camera = renderCamera != null ? renderCamera : Camera.main;
            if (camera == null)
                return;

            camera.AddCommandBuffer(CameraEvent.AfterEverything, commandBuffer);
            attachedCamera = camera;
            cameraCommandBufferAttached = true;
        }

        private void DetachBuiltInCommandBuffer()
        {
            if (!cameraCommandBufferAttached || attachedCamera == null || commandBuffer == null)
                return;
            attachedCamera.RemoveCommandBuffer(CameraEvent.AfterEverything, commandBuffer);
            attachedCamera = null;
            cameraCommandBufferAttached = false;
        }
    }
}
