/* Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
using System;
using MDK.Unity;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.HighDefinition;

[Serializable]
public sealed class MDKHDRComposite : CustomPass
{
    public MDKPlayer player;
    public Shader compositeShader;
    private Material material;
    private bool logged;
    protected override void Setup(ScriptableRenderContext renderContext, CommandBuffer cmd)
    {
        material = CoreUtils.CreateEngineMaterial(compositeShader);
    }
    protected override void Execute(CustomPassContext ctx)
    {
        if (player == null || player.VideoTexture == null || player.OutputMode != MDKOutputMode.HDRLinear)
            return;
        ctx.propertyBlock.SetTexture("_Video", player.VideoTexture);
        CoreUtils.SetRenderTarget(ctx.cmd, ctx.cameraColorBuffer, ClearFlag.None);
        CoreUtils.DrawFullScreen(ctx.cmd, material, ctx.propertyBlock, shaderPassId: 0);
        if (!logged && player.AppliedTextureVersion == player.TargetVersion) {
            Debug.Log("MDK_HDR_COMPOSITE BeforePostProcess scRGB to HDRP pre-exposed luminance");
            logged = true;
        }
    }
    protected override void Cleanup() { CoreUtils.Destroy(material); }
}
