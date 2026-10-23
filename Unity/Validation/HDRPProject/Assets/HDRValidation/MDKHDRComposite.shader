/* Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
Shader "MDK/HDRP scRGB Composite"
{
    SubShader {
        Tags { "RenderPipeline"="HDRenderPipeline" }
        Pass {
            ZWrite Off ZTest Always Cull Off Blend Off
            HLSLPROGRAM
            #pragma vertex Vert
            #pragma fragment Frag
            #pragma target 4.5
            #include "Packages/com.unity.render-pipelines.high-definition/Runtime/RenderPipeline/RenderPass/CustomPass/CustomPassCommon.hlsl"
            TEXTURE2D(_Video);
            SAMPLER(sampler_Video);
            float4 Frag(Varyings i) : SV_Target {
                UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX(i);
                float2 uv = i.positionCS.xy * _ScreenSize.zw;
                #if UNITY_UV_STARTS_AT_TOP
                uv.y = 1 - uv.y;
                #endif
                float3 scRGB = SAMPLE_TEXTURE2D_LOD(_Video, sampler_Video, uv, 0).rgb;
// HDRP HDR LUT input uses 1=100 nit; scRGB uses 1=80 nit. Follow pre-exposure like emissive scene content.
                return float4(scRGB * 0.8 * GetCurrentExposureMultiplier(), 1);
            }
            ENDHLSL
        }
    }
}
