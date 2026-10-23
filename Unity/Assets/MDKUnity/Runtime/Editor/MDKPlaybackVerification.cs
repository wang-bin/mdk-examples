/*
 * Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
#if UNITY_EDITOR
using System;
using System.IO;
using System.Globalization;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.UI;

namespace MDK.Unity.Editor
{
    // Run inside Unity so the real camera command buffer and display bindings are exercised.
    [InitializeOnLoad]
    public static class MDKPlaybackVerification
    {
        private const string ActiveKey = "MDK.Verify.Active";
        private const string StartKey = "MDK.Verify.Start";
        private const string ExitCodeKey = "MDK.Verify.ExitCode";
        private static double nextSample;
        private static uint firstHash;
        private static int samples;
        private static int hdrPhase;
        private static ulong hdrVersion;
        private static MDKPlayer secondPlayer;

        static MDKPlaybackVerification()
        {
            EditorApplication.playModeStateChanged += OnPlayModeChanged;
            if (SessionState.GetBool(ActiveKey, false))
                EditorApplication.update += Tick;
        }

        private static void OnPlayModeChanged(PlayModeStateChange state)
        {
            if (state != PlayModeStateChange.EnteredEditMode)
                return;
            var code = SessionState.GetInt(ExitCodeKey, -1);
            if (code < 0)
                return;
            SessionState.SetInt(ExitCodeKey, -1);
            EditorApplication.delayCall += () => EditorApplication.Exit(code);
        }

        [MenuItem("MDK/Verify sample playback")]
        public static void Run()
        {
            if (EditorApplication.isPlayingOrWillChangePlaymode)
                throw new InvalidOperationException("Stop Play Mode before running verification.");
            if (!Application.isBatchMode && !EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo())
                return;
            var scene = Environment.GetEnvironmentVariable("MDK_UNITY_VERIFY_SCENE");
            if (string.IsNullOrEmpty(scene))
                scene = "Assets/MDKUnity/Samples/Scenes/MDKPlayer.unity";
            EditorSceneManager.OpenScene(scene);
            var player = UnityEngine.Object.FindObjectOfType<MDKPlayer>();
            var media = Environment.GetEnvironmentVariable("MDK_UNITY_VERIFY_MEDIA");
            if (string.IsNullOrEmpty(media))
                media = Path.GetFullPath(Path.Combine(Application.dataPath,
                    "../../flutter/fvp-test-fade/assets/videos/Big_Buck_Bunny_1080_10s_1MB.mp4"));
            if (!File.Exists(media))
                throw new FileNotFoundException("Verification media was not found", media);
            player.MediaUrl = media;
            if (Environment.GetEnvironmentVariable("MDK_UNITY_VERIFY_HDR") == "1")
                player.OutputMode = MDKOutputMode.HDRLinear;
            samples = 0;
            hdrPhase = 0;
            firstHash = 0;
            nextSample = 0;
            SessionState.SetBool(ActiveKey, true);
            SessionState.SetInt(ExitCodeKey, -1);
            SessionState.SetString(StartKey, DateTime.UtcNow.ToString("O"));
            Debug.Log($"MDK_VERIFY start scene={scene} media={media}");
            EditorApplication.update -= Tick;
            EditorApplication.update += Tick;
            EditorApplication.isPlaying = true;
        }

        private static void Tick()
        {
            if ((DateTime.UtcNow - DateTime.Parse(SessionState.GetString(StartKey, ""),
                CultureInfo.InvariantCulture, DateTimeStyles.RoundtripKind)).TotalSeconds > 60) {
                Finish(false, "Timed out waiting for video pixels");
                return;
            }
            if (!EditorApplication.isPlaying || EditorApplication.isCompiling ||
                EditorApplication.timeSinceStartup < nextSample)
                return;
            nextSample = EditorApplication.timeSinceStartup + 1;
            // Batch-mode Editor has no Game view driving camera rendering.
            if (Application.isBatchMode && Camera.main != null)
                Camera.main.Render();
            var player = UnityEngine.Object.FindObjectOfType<MDKPlayer>();
            if (player == null || player.VideoTexture == null || (player.Position < 1000 && hdrPhase != 3 && hdrPhase != 4))
                return;
            var rt = player.VideoTexture;
            if (Environment.GetEnvironmentVariable("MDK_UNITY_VERIFY_HDR") == "1") {
                if (hdrPhase == 1) {
                    if (player.AppliedTextureVersion != player.TargetVersion) return;
                    if (rt.format != RenderTextureFormat.ARGB32) { Finish(false, "SDR switch did not produce ARGB32"); return; }
                    player.OutputMode = MDKOutputMode.HDRLinear;
                    hdrPhase = 2;
                    return;
                }
                VerifyHDR(player);
                return;
            }
            var previous = RenderTexture.active;
            var readback = new Texture2D(rt.width, rt.height, TextureFormat.RGBA32, false);
            RenderTexture.active = rt;
            readback.ReadPixels(new Rect(0, 0, rt.width, rt.height), 0, 0);
            readback.Apply();
            RenderTexture.active = previous;
            var pixels = readback.GetPixels32();
            uint hash = 2166136261;
            int lit = 0;
            int opaque = 0;
            for (var i = 0; i < pixels.Length; i += 97) {
                var p = pixels[i];
                if (p.r > 8 || p.g > 8 || p.b > 8) ++lit;
                if (p.a > 128) ++opaque;
                hash = unchecked((hash ^ p.r ^ ((uint)p.g << 8) ^ ((uint)p.b << 16)) * 16777619);
            }
            var image = UnityEngine.Object.FindObjectOfType<RawImage>();
            bool bound = image != null && image.texture == rt && image.isActiveAndEnabled &&
                image.rectTransform.rect.width > 0 && image.rectTransform.rect.height > 0;
            var mesh = UnityEngine.Object.FindObjectOfType<MDKMeshDisplay>();
            if (mesh != null) {
                var block = new MaterialPropertyBlock();
                var renderer = mesh.GetComponent<Renderer>();
                renderer.GetPropertyBlock(block);
                bound = renderer.enabled && block.GetTexture("_MainTex") == rt &&
                    renderer.sharedMaterial != null && renderer.sharedMaterial.shader.isSupported;
            }
            Debug.Log($"MDK_VERIFY sample renderer={player.Renderer} size={rt.width}x{rt.height} position={player.Position} version={player.AppliedTextureVersion}/{player.TargetVersion} frame={Time.frameCount} camera={Camera.main} lit={lit} opaque={opaque} hash={hash} displayBound={bound} error={player.ErrorMessage}");
            var output = Environment.GetEnvironmentVariable("MDK_UNITY_VERIFY_OUTPUT");
            if (string.IsNullOrEmpty(output)) output = "Temp/MDKVerification";
            Directory.CreateDirectory(output);
            File.WriteAllBytes(Path.Combine(output, "video-" + samples + ".png"), readback.EncodeToPNG());
            UnityEngine.Object.DestroyImmediate(readback);
            if (lit == 0 || !bound) return;
            if (samples++ == 0) {
                firstHash = hash;
                // CaptureScreenshot is unavailable in batch mode on some Unity versions.
                if (!Application.isBatchMode)
                    ScreenCapture.CaptureScreenshot(Path.Combine(output, "game.png"));
            } else if (hash != firstHash) {
                if (!Application.isBatchMode) {
                    var screenshot = Path.Combine(output, "game.png");
                    if (!File.Exists(screenshot)) return;
                    var game = new Texture2D(2, 2);
                    game.LoadImage(File.ReadAllBytes(screenshot));
                    int colorful = 0;
                    foreach (var p in game.GetPixels32()) {
                        if (Mathf.Max(p.r, Mathf.Max(p.g, p.b)) - Mathf.Min(p.r, Mathf.Min(p.g, p.b)) > 20)
                            ++colorful;
                    }
                    UnityEngine.Object.DestroyImmediate(game);
                    if (colorful < 100) {
                        Finish(false, "Video texture is valid but Game screenshot has no video colors");
                        return;
                    }
                    Debug.Log($"MDK_VERIFY Game screenshot colorfulPixels={colorful}");
                }
                Finish(true, "Video pixels change and display texture is bound");
            }
        }

        private static void Finish(bool passed, string reason)
        {
            SessionState.SetBool(ActiveKey, false);
            EditorApplication.update -= Tick;
            Debug.Log($"MDK_VERIFY {(passed ? "PASS" : "FAIL")} {reason}");
            if (Application.isBatchMode || Environment.GetEnvironmentVariable("MDK_UNITY_VERIFY_EXIT") == "1")
                SessionState.SetInt(ExitCodeKey, passed ? 0 : 1);
            // Allow OnDestroy and queued rendering events to finish before quitting the Editor.
            EditorApplication.isPlaying = false;
        }

        private static void VerifyHDR(MDKPlayer player)
        {
            if (hdrPhase == 4) {
                if (player.State != MDKNative.StateStopped) { Finish(false, "Stop failed"); return; }
                player.Play();
                var go = new GameObject("Second HDR verification player");
                go.SetActive(false);
                secondPlayer = go.AddComponent<MDKPlayer>();
                secondPlayer.OutputMode = MDKOutputMode.HDRLinear;
                secondPlayer.MediaUrl = player.MediaUrl;
                go.SetActive(true);
                hdrPhase = 5;
                return;
            }
            var rt = player.VideoTexture;
            if (player.HasError) { Finish(false, player.ErrorMessage); return; }
            if (player.AppliedTextureVersion != player.TargetVersion) return;
            var previous = RenderTexture.active;
            var readback = new Texture2D(rt.width, rt.height, TextureFormat.RGBAFloat, false, true);
            RenderTexture.active = rt;
            readback.ReadPixels(new Rect(0, 0, rt.width, rt.height), 0, 0);
            readback.Apply();
            RenderTexture.active = previous;
            var nits = new[] { 0f, 80f, 100f, 203f, 1000f };
            bool passed = rt.graphicsFormat == UnityEngine.Experimental.Rendering.GraphicsFormat.R16G16B16A16_SFloat && !rt.sRGB;
            for (int i = 0; i < nits.Length; ++i) {
                var color = readback.GetPixel((2 * i + 1) * rt.width / 10, rt.height / 2);
                var expected = nits[i] / 80f;
                var tolerance = Mathf.Max(0.005f, expected * 0.03f);
                passed &= Mathf.Abs(color.r - expected) <= tolerance && Mathf.Abs(color.g - expected) <= tolerance && Mathf.Abs(color.b - expected) <= tolerance;
                Debug.Log($"MDK_HDR patch={nits[i]}nit expected={expected:R} actual={color:R} format={rt.graphicsFormat} sRGB={rt.sRGB}");
            }
            UnityEngine.Object.DestroyImmediate(readback);
            if (!passed) { Finish(false, "HDR scRGB FP16 numerical calibration (3% tolerance)"); return; }
            if (hdrPhase == 0) {
                hdrVersion = player.TargetVersion;
                player.OutputMode = MDKOutputMode.SDR;
                hdrPhase = 1;
            } else if (hdrPhase == 2) {
                if (player.TargetVersion <= hdrVersion + 1) { Finish(false, "HDR switch did not advance generation"); return; }
                player.Pause();
                hdrPhase = 3;
            } else if (hdrPhase == 3) {
                if (player.State != MDKNative.StatePaused) { Finish(false, "Pause failed"); return; }
                player.Stop();
                hdrPhase = 4;
            } else if (hdrPhase == 5) {
                if (secondPlayer.HasError) { Finish(false, secondPlayer.ErrorMessage); return; }
                if (secondPlayer.Position < 1000 || secondPlayer.AppliedTextureVersion != secondPlayer.TargetVersion) return;
                Debug.Log($"MDK_HDR lifecycle secondPlayer size={secondPlayer.VideoTexture.width}x{secondPlayer.VideoTexture.height} applied={secondPlayer.AppliedTextureVersion}");
                UnityEngine.Object.Destroy(secondPlayer.gameObject);
                var resizeMedia = Environment.GetEnvironmentVariable("MDK_UNITY_VERIFY_RESIZE_MEDIA");
                if (string.IsNullOrEmpty(resizeMedia) || !File.Exists(resizeMedia)) {
                    Finish(false, "HDR lifecycle requires MDK_UNITY_VERIFY_RESIZE_MEDIA fixture"); return;
                }
                hdrVersion = player.TargetVersion;
                player.Open(resizeMedia);
                hdrPhase = 6;
            } else {
                if (player.TargetVersion == hdrVersion) return;
                Finish(true, "HDR scRGB calibration, SDR/HDR switches, pause, stop/replay, two players, media resize and Play Mode exit");
            }
        }
    }
}
#endif
