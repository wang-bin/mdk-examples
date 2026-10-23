/*
 * Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
using System;
using System.Runtime.InteropServices;

namespace MDK.Unity
{
    public static class MDKNative
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct RenderTargetV2
        {
            public uint structSize;
            public MDKOutputMode outputMode;
            public IntPtr nativeTexture;
            public int width;
            public int height;
            public ulong version;
        }

#if UNITY_IOS && !UNITY_EDITOR
        private const string Library = "__Internal";
#elif UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN || UNITY_WSA
        private const string Library = "MDKUnityPlugin";
#elif UNITY_ANDROID
        private const string Library = "MDKUnityPlugin";
#else
        private const string Library = "MDKUnityPlugin";
#endif

        [StructLayout(LayoutKind.Sequential)]
        public struct PlayerInfo
        {
            public int state;
            public int mediaStatus;
            public int width;
            public int height;
            public float pixelAspect;
            public int rotation;
            public long position;
            public long duration;
            public ulong targetVersion;
            public ulong appliedVersion;
            public int renderer;
            public int error;
        }

        [DllImport(Library, CallingConvention = CallingConvention.Winapi)]
        public static extern ulong MDKUnityPlayerCreate();

        [DllImport(Library, CallingConvention = CallingConvention.Winapi)]
        public static extern void MDKUnityPlayerDestroy(ulong handle);

        [DllImport(Library, CallingConvention = CallingConvention.Winapi)]
        public static extern void MDKUnityPlayerSetMedia(ulong handle, [MarshalAs(UnmanagedType.LPUTF8Str)] string url);

        [DllImport(Library, CallingConvention = CallingConvention.Winapi)]
        public static extern void MDKUnityPlayerSetState(ulong handle, int state);

        [DllImport(Library, CallingConvention = CallingConvention.Winapi)]
        public static extern void MDKUnityPlayerSetLoop(ulong handle, int count);

        [DllImport(Library, CallingConvention = CallingConvention.Winapi)]
        public static extern void MDKUnityPlayerSetRenderTarget(ulong handle, IntPtr texture, int width, int height, ulong version);

        [DllImport(Library, CallingConvention = CallingConvention.Winapi)]
        public static extern int MDKUnityPlayerSetRenderTargetV2(ulong handle, ref RenderTargetV2 target);

        [DllImport(Library, CallingConvention = CallingConvention.Winapi)]
        public static extern void MDKUnityPlayerGetInfo(ulong handle, out PlayerInfo info);

        [DllImport(Library, CallingConvention = CallingConvention.Winapi)]
        public static extern int MDKUnityPlayerCopyError(ulong handle, IntPtr buffer, int capacity);

        [DllImport(Library, CallingConvention = CallingConvention.Winapi)]
        public static extern IntPtr MDKUnityGetRenderEventFunc();

        [DllImport(Library, CallingConvention = CallingConvention.Winapi)]
        public static extern int MDKUnityGetRenderer();

        internal const int StateStopped = 0;
        internal const int StatePlaying = 1;
        internal const int StatePaused = 2;
        internal const int MediaLoaded = 1 << 2;
        internal const int MediaInvalid = unchecked((int)0x80000000);
        internal const int RenderEvent = 1;
        internal const int ReleaseEvent = 2;
    }
}
