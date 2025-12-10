#include "CopyImage.hlsli"

Texture2D<float4> gSceneTex : register(t0);
SamplerState gSampler : register(s0);

cbuffer RippleCB : register(b0)
{
    float2 CenterUV; // 中心
    float Radius; // 半径
    float Amplitude; // 歪み量

    float Frequency; // 細かさ
    float BandWidth; // リングの幅
    float Padding; // DirectXCommon::WaterRippleCB の padding と合わせる

    float3 RippleColor; // 波紋の色 (RGB)
    float ColorIntensity; // どれだけ色を足すか
}

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float2 uv = input.texcoord;
    float dist = distance(uv, CenterUV);

    // リングの強度（dist ≒ Radius のところだけ強くする）
    float ring = exp(-pow((dist - Radius) * BandWidth, 2));

    // 波紋（sin）はリング付近だけ効くように
    float ripple = sin(dist * Frequency - Radius) * Amplitude * ring;

    // 歪みもリングだけ
    float2 dir = (dist > 0.0001f)
        ? normalize(uv - CenterUV)
        : float2(0.0f, 0.0f);

    float2 distortedUV = uv + dir * ripple;

    // 元画像
    float4 col = gSceneTex.Sample(gSampler, distortedUV);

    // 色もリングに乗せる
    col.rgb += RippleColor * ring * ColorIntensity;

    return col;
}