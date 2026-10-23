/* Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
using NUnit.Framework;
using System.Runtime.InteropServices;

namespace MDK.Unity.Tests
{
    public sealed class MDKNativeTests
    {
        [Test]
        public void VersionedTargetRejectsIncompleteDescriptors()
        {
            var handle = MDKNative.MDKUnityPlayerCreate();
            try {
                var target = new MDKNative.RenderTargetV2();
                Assert.That(MDKNative.MDKUnityPlayerSetRenderTargetV2(handle, ref target), Is.Zero);
                MDKNative.MDKUnityPlayerGetInfo(handle, out var info);
                Assert.That(info.targetVersion, Is.Zero);
                Assert.That(info.error, Is.Not.Zero);
                Assert.That(Marshal.OffsetOf<MDKNative.RenderTargetV2>("nativeTexture").ToInt32(), Is.EqualTo(8));
                Assert.That(Marshal.SizeOf<MDKNative.PlayerInfo>(), Is.EqualTo(64));
                if (System.IntPtr.Size == 8)
                    Assert.That(Marshal.SizeOf<MDKNative.RenderTargetV2>(), Is.EqualTo(32));
            } finally {
                MDKNative.MDKUnityPlayerDestroy(handle);
            }
        }
        [Test]
        public void NativeHandleCanBeCreatedAndDestroyed()
        {
            ulong handle;
            try {
                handle = MDKNative.MDKUnityPlayerCreate();
            } catch (System.DllNotFoundException) {
                Assert.Ignore("Build MDKUnityPlugin and copy it to Assets/Plugins before running native tests.");
                return;
            }
            Assert.That(handle, Is.GreaterThan(0UL));
            MDKNative.MDKUnityPlayerGetInfo(handle, out var info);
            Assert.That(info.targetVersion, Is.EqualTo(0UL));
            MDKNative.MDKUnityPlayerDestroy(handle);
        }
    }
}
