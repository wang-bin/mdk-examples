/*
 * Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
Shader "MDK/Unlit Video"
{
    Properties
    {
        _MainTex ("Video Texture", 2D) = "black" {}
        _Color ("Color", Color) = (1, 1, 1, 1)
        _HDRPreview ("scRGB SDR Preview", Float) = 0
    }

    SubShader
    {
        Tags { "RenderType" = "Opaque" "Queue" = "Geometry" }
        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct v2f
            {
                float4 vertex : SV_POSITION;
                float2 uv : TEXCOORD0;
            };

            sampler2D _MainTex;
            float4 _MainTex_ST;
            float4 _Color;
            float _HDRPreview;

            v2f vert(appdata input)
            {
                v2f output;
                output.vertex = UnityObjectToClipPos(input.vertex);
                output.uv = TRANSFORM_TEX(input.uv, _MainTex);
                return output;
            }

            float4 frag(v2f input) : SV_Target
            {
                float4 c = tex2D(_MainTex, input.uv) * _Color;
                if (_HDRPreview > 0.5) {
                    c.rgb = max(c.rgb, 0) / (1 + max(c.rgb, 0));
                    #ifdef UNITY_COLORSPACE_GAMMA
                    c.rgb = LinearToGammaSpace(c.rgb);
                    #endif
                }
                return c;
            }
            ENDCG
        }
    }
}
