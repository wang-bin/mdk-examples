/* Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
using System;
using System.IO;
using MDK.Unity;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.HighDefinition;

public static class MDKHDRProject
{
    private const string ScenePath = "Assets/Generated/HDRValidation.unity";
    [MenuItem("MDK/HDRP/Create validation scene")]
    public static void CreateScene()
    {
        var existingPlayer = UnityEngine.Object.FindObjectOfType<MDKPlayer>();
        var media = Environment.GetEnvironmentVariable("MDK_UNITY_VERIFY_MEDIA") ??
            (existingPlayer != null ? existingPlayer.MediaUrl : "");
        Directory.CreateDirectory("Assets/Generated");
        PlayerSettings.companyName = "MDK";
        PlayerSettings.productName = "MDK HDR Validation";
        PlayerSettings.colorSpace = ColorSpace.Linear;
        PlayerSettings.SetUseDefaultGraphicsAPIs(BuildTarget.StandaloneWindows64, false);
        PlayerSettings.SetGraphicsAPIs(BuildTarget.StandaloneWindows64, new[] { GraphicsDeviceType.Direct3D12 });
        PlayerSettings.allowHDRDisplaySupport = true;
        PlayerSettings.useHDRDisplay = true;
        var asset = AssetDatabase.LoadAssetAtPath<HDRenderPipelineAsset>("Assets/Generated/HDR.asset");
        if (asset == null) {
            asset = ScriptableObject.CreateInstance<HDRenderPipelineAsset>();
            AssetDatabase.CreateAsset(asset, "Assets/Generated/HDR.asset");
        }
        var settings = asset.currentPlatformRenderPipelineSettings;
        settings.colorBufferFormat = RenderPipelineSettings.ColorBufferFormat.R16G16B16A16;
        settings.supportCustomPass = true;
        asset.currentPlatformRenderPipelineSettings = settings;
        EditorUtility.SetDirty(asset);
        GraphicsSettings.renderPipelineAsset = asset;
        QualitySettings.renderPipeline = asset;
        EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
        var cameraObject = new GameObject("Main Camera", typeof(Camera), typeof(HDAdditionalCameraData));
        cameraObject.tag = "MainCamera";
        var cameraData = cameraObject.GetComponent<HDAdditionalCameraData>();
        cameraData.antialiasing = HDAdditionalCameraData.AntialiasingMode.None;
        var player = new GameObject("MDK HDR Player").AddComponent<MDKPlayer>();
        player.OutputMode = MDKOutputMode.HDRLinear;
        player.MediaUrl = media;
        var custom = new GameObject("BeforePostProcess video").AddComponent<CustomPassVolume>();
        custom.isGlobal = true;
        custom.injectionPoint = CustomPassInjectionPoint.BeforePostProcess;
        custom.customPasses.Add(new MDKHDRComposite { player = player, compositeShader = Shader.Find("MDK/HDRP scRGB Composite") });
        var profile = ScriptableObject.CreateInstance<VolumeProfile>();
        var exposure = profile.Add<Exposure>(true);
        exposure.mode.Override(ExposureMode.Fixed);
        exposure.fixedExposure.Override(0);
        var tonemapping = profile.Add<Tonemapping>(true);
        tonemapping.mode.Override(TonemappingMode.Neutral);
        profile.Add<Bloom>(true).intensity.Override(0);
        profile.Add<ColorAdjustments>(true).postExposure.Override(0);
        AssetDatabase.CreateAsset(profile, AssetDatabase.GenerateUniqueAssetPath("Assets/Generated/Calibration.asset"));
        var volume = new GameObject("Fixed exposure calibration").AddComponent<Volume>();
        volume.isGlobal = true;
        volume.priority = 1000;
        volume.sharedProfile = profile;
        var status = new GameObject("HDR status").AddComponent<MDKHDROutputStatus>();
        status.requestedMode = PlayerSettings.hdrBitDepth.ToString();
        status.player = player;
        EditorSceneManager.SaveScene(UnityEngine.SceneManagement.SceneManager.GetActiveScene(), ScenePath);
        EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(ScenePath, true) };
        AssetDatabase.SaveAssets();
    }

    [MenuItem("MDK/HDRP/Build scRGB Player")]
    public static void BuildSCRGB() { Build(HDRDisplayBitDepth.BitDepth16); }
    [MenuItem("MDK/HDRP/Build HDR10 Player")]
    public static void BuildHDR10() { Build(HDRDisplayBitDepth.BitDepth10); }
    [MenuItem("MDK/HDRP/Build both Players")]
    public static void BuildBoth() { BuildSCRGB(); BuildHDR10(); }
    private static void Build(HDRDisplayBitDepth depth)
    {
        var package = UnityEditor.PackageManager.PackageInfo.FindForAssembly(typeof(HDRenderPipelineAsset).Assembly);
        if (package.version != "14.0.12")
            throw new InvalidOperationException("HDR validation requires HDRP 14.0.12; resolved " + package.version + ". Use prepare-hdrp.ps1 -UnityEditorRoot to pin official packages in Tuanjie.");
        PlayerSettings.hdrBitDepth = depth;
        CreateScene();
        var repo = Path.GetFullPath(Path.Combine(Application.dataPath, "../../../../.."));
        var output = Path.Combine(repo, "build/cl64/unity-hdr", depth.ToString(), "MDKHDR.exe");
        Directory.CreateDirectory(Path.GetDirectoryName(output));
        var report = BuildPipeline.BuildPlayer(new[] { ScenePath }, output, BuildTarget.StandaloneWindows64, BuildOptions.Development);
        if (report.summary.result != BuildResult.Succeeded)
            throw new InvalidOperationException("HDRP Player build failed: " + report.summary.result);
    }
}
