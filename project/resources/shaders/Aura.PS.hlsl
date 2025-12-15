#include "CopyImage.hlsli"

Texture2D<float4> gSceneTex : register(t0);
SamplerState gSampler : register(s0);

cbuffer AuraCB : register(b0)
{
    float2 CenterUV;
    float Time;
    float Scale;

    float Intensity;
    float UseRing;
    float RingRadius;
    float RingWidth;

    float3 ColorA;
    float _pad0;

    float3 ColorB;
    float Mix;
};

struct VSOut
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 main(VSOut input) : SV_TARGET
{
    float2 uv = input.texcoord;
    float4 scene = gSceneTex.Sample(gSampler, uv);

    // 中心からの距離（Scale で広がり調整）
    float2 d = (uv - CenterUV) / max(0.0001, Scale);
    float r = length(d);

    // ふわっとしたコア
    float core = exp(-r * 2.2);

    // 縁の発光（リングっぽい外周）
    float edge = exp(-abs(r - 0.85) * 6.0);

    // 足元リング（UseRing==1で有効）
    float ring = 0.0;
    if (UseRing > 0.5)
    {
        float rr = distance(uv, CenterUV);
        ring = exp(-pow((rr - RingRadius) * RingWidth, 2.0));
    }

    float3 col = lerp(ColorA, ColorB, saturate(Mix));

    float aura = (core + edge * 0.75 + ring * 0.9) * Intensity;

    // 加算合成
    scene.rgb += col * aura;

    return scene;
}