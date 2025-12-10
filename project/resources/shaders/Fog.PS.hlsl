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
    float NoiseStrength; // 「濃さのムラ」の強さ

    float Time; // 経過時間
    float _pad;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// ----------------------------
// より滑らかな value noise
// ----------------------------
float hash21(float2 p)
{
    // グリッドベースのハッシュ
    p = frac(p * 0.3183099 + float2(0.71, 0.113));
    p *= 17.0;
    return frac(p.x * p.y * (p.x + p.y));
}

// 補間付き value noise
float smoothNoise2D(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    float a = hash21(i);
    float b = hash21(i + float2(1.0, 0.0));
    float c = hash21(i + float2(0.0, 1.0));
    float d = hash21(i + float2(1.0, 1.0));

    float2 u = f * f * (3.0 - 2.0 * f); // smoothstep

    float x1 = lerp(a, b, u.x);
    float x2 = lerp(c, d, u.x);
    return lerp(x1, x2, u.y); // 0〜1
}

// fbm っぽく少し階層を足す
float fbm2D(float2 p)
{
    float n = 0.0f;
    float a = 0.5f;
    float f = 1.0f;

    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        n += smoothNoise2D(p * f) * a;
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
    // 1. 高さベースの霧の「ベース量」
    // ----------------------------

    // 画面下を 0、上を 1 とした高さ
    float height = 1.0f - uv.y;

    // FogStart〜FogEnd の範囲で 0→1 に立ち上がる
    float base = 0.0f;
    if (FogEnd > FogStart + 1e-5f)
    {
        base = saturate((height - FogStart) / (FogEnd - FogStart));
    }
    base = smoothstep(0.0f, 1.0f, base);

    // ----------------------------
    // 2. 「空気の塊」による濃さのムラ（大きめノイズ）
    // ----------------------------

    float ns = saturate(NoiseStrength); // 0〜1

    // ノイズスケールが極端にでかいと模様になるので、ある程度クランプ
    float baseScale = lerp(0.8f, 3.0f, saturate(NoiseScale * 0.1f));

    // 風向き（右上に流れる感じ）
    float2 wind = normalize(float2(0.6f, 0.2f));

    // 大きめの fbm ノイズ（霧の塊の形）
    float2 bigCoord =
        uv * baseScale +
        wind * Time * 0.02f;

    float bigNoise = fbm2D(bigCoord); // 0〜1

    // 「どこでも最低限の霧 + 所々だけ濃い塊」を作る
    float minFog = 0.3f; // 常にある薄い霧の量
    float threshold = 0.45f; // 塊になり始める境目
    float softness = 0.15f; // 塊の境界の柔らかさ

    // bigNoise が threshold を超えたところが「塊」
    float patch = smoothstep(threshold,
                             threshold + softness,
                             bigNoise);

    // ns=0 なら塊効果なし、ns=1 で patch フル適用
    float patchFactor = lerp(0.0f, patch, ns);

    // coverage = 薄い霧 + 塊で増える分
    float coverage = minFog + patchFactor * (1.0f - minFog);

    // ----------------------------
    // 3. 細かいゆらぎ（空気の流れ）
    // ----------------------------

    float2 detailCoord =
        uv * (baseScale * 3.0f) +
        wind * Time * 0.07f;

    float detailNoise = fbm2D(detailCoord); // 0〜1
    float detailFactor = lerp(0.9f, 1.1f, detailNoise); // ほんの少しだけ濃淡

    // ----------------------------
    // 4. 霧量の合成
    // ----------------------------

    // 高さベース × coverage（塊マスク） × 細かい揺らぎ
    float fogRaw = base * coverage * detailFactor;

    // 全体の濃さ
    float fogAmount = fogRaw * FogDensity;

    // 全面真っ白は避けたいので上限を抑える
    fogAmount = min(fogAmount, 0.7f);

    // 霧らしくするための非線形
    fogAmount = saturate(1.0f - exp(-fogAmount));

    // ----------------------------
    // 5. 色をブレンド
    // ----------------------------

    float3 fogged = lerp(scene.rgb, FogColor, fogAmount);

    output.color.rgb = fogged;
    output.color.a = scene.a;
    return output;
}
