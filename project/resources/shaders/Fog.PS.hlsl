#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// DirectXCommon::FogCB とレイアウトを合わせる
cbuffer FogCB : register(b0)
{
    float3 FogColor; // 霧の色
    float FogDensity; // 全体の濃さ

    float FogStart; // 霧が出始める高さ(0〜1)  画面下からの距離
    float FogEnd; // 霧が最大になる高さ(0〜1)

    float NoiseScale; // ノイズの細かさ
    float NoiseStrength; // ノイズによるゆらぎの強さ

    float Time; // 経過時間
    float _pad;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// ----------------------------
// シンプルな value noise (fbm)
// ----------------------------
float hash21(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float noise2D(float2 p)
{
    // ちょっとだけ滑らかにするための fbm っぽいやつ
    float n = 0.0f;
    float a = 0.5f;
    float f = 1.0f;

    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        n += hash21(p * f) * a;
        f *= 2.0f;
        a *= 0.5f;
    }
    return n; // 0〜1
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;
    float4 scene = gTexture.Sample(gSampler, uv);

    // ----------------------------
    // 1. 高さベースの霧の基本量
    // ----------------------------

    // 画面下を 0、上を 1 とした「高さ」
    float height = 1.0f - uv.y;

    // FogStart〜FogEnd の範囲で 0→1 に立ち上がる
    float base = saturate((height - FogStart) / max(FogEnd - FogStart, 1e-5));

    // そのままだとカクッと立ち上がるので smoothstep で柔らかく
    base = smoothstep(0.0f, 1.0f, base);

    // ----------------------------
    // 2. ゆらぎ用のノイズ
    // ----------------------------
    float2 ncoord = uv * NoiseScale
                  + float2(Time * 0.03f, Time * 0.017f);

    float n = noise2D(ncoord); // 0〜1
    // -0.5〜+0.5 くらいの変化に抑える
    float wobble = (n - 0.5f) * 2.0f;

    // ノイズの強さを反映
    float fogNoise = base + wobble * NoiseStrength;
    fogNoise = saturate(fogNoise);

    // ----------------------------
    // 3. 濃さ＆色ブレンド
    // ----------------------------
    float fogAmount = fogNoise * FogDensity;

    // ちょっとだけ指数的に強調すると「もや」っぽくなる
    fogAmount = saturate(1.0f - exp(-fogAmount));

    float3 fogged = lerp(scene.rgb, FogColor, fogAmount);

    output.color.rgb = fogged;
    output.color.a = scene.a;
    return output;
}
