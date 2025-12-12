#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// DirectXCommon::FogCB とレイアウトを合わせる
cbuffer FogCB : register(b0)
{
    float3 FogColor;
    float FogDensity;

    float FogStart;
    float FogEnd;

    float NoiseScale;
    float NoiseStrength;

    float Time;
    float WorldScale;

    float3 WorldPos;
    float _pad;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// ----------------------------
// ハッシュ → value noise 用
// ----------------------------
float hash21(float2 p)
{
    p = frac(p * 0.3183099 + float2(0.71, 0.113));
    p *= 17.0;
    return frac(p.x * p.y * (p.x + p.y));
}

float smoothNoise2D(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    float a = hash21(i);
    float b = hash21(i + float2(1.0, 0.0));
    float c = hash21(i + float2(0.0, 1.0));
    float d = hash21(i + float2(1.0, 1.0));

    float2 u = f * f * (3.0 - 2.0 * f);

    float x1 = lerp(a, b, u.x);
    float x2 = lerp(c, d, u.x);
    return lerp(x1, x2, u.y);
}

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
    return n;
}

// ----------------------------
// 擬似 "ボリューム" 霧レイヤー
// ★ 空間固定感を強める版
// ----------------------------
float integrateFogLayers(float2 uv, float height, float ns, float time)
{
    const int STEPS = 6;
    const float STEP_INV = 1.0f / STEPS;

    float2 wind = normalize(float2(0.6f, 0.25f));

    // NoiseScale はそのまま使う（大きいほど細かい）
    float baseScale = max(0.01f, NoiseScale);

    // ★ 世界位置（XZ）をノイズ座標に変換
    //   ここが「空間に固定」される参照点
    float2 worldBase = float2(WorldPos.x, WorldPos.z) * WorldScale;

    float accum = 0.0f;

    [unroll]
    for (int i = 0; i < STEPS; ++i)
    {
        float t = (i + 0.5f) * STEP_INV;

        // レイヤーごとにスケールと影響度を変える
        float layerScale = baseScale * lerp(0.9f, 1.6f, t);

        // ★ 重要：uv由来を弱めて、world由来を主役にする
        //    uvの係数を下げると貼り付き感が減る
        float2 uvPart = uv * layerScale * 0.65f;

        // ★ 重要：手前ほど world の影響を強く、奥ほど弱く
        float worldWeight = lerp(1.35f, 0.55f, t);

        // 高さ寄与（地面付近の密度変化のため）
        float heightOffset = height * (0.5f + t);

        // ちょい蛇行（世界側の座標にも効く）
        float2 wobble = float2(
            sin(time * 0.35f + t * 4.1f),
            cos(time * 0.29f + t * 5.3f)
        ) * 0.12f;

        // レイヤーのノイズ座標
        float2 coord =
            uvPart
            + worldBase * worldWeight
            + wobble
            + wind * (time * 0.06f + t * 2.8f)
            + float2(heightOffset * 0.75f, -heightOffset * 0.55f);

        float n = fbm2D(coord);

        // 「塊」だけを抜く
        float threshold = 0.46f;
        float softness = 0.18f;
        float patch = smoothstep(threshold, threshold + softness, n);

        // 呼吸
        float breatheNoise = fbm2D(coord * 0.55f + float2(time * 0.07f, -time * 0.05f));
        float breathe = lerp(0.75f, 1.35f, breatheNoise);
        patch *= breathe;

        // 高さが低いほど濃い
        float heightWeight = saturate(1.0f - height * 0.85f);
        patch *= heightWeight;

        // ノイズ強度
        patch *= ns;

        accum += patch * STEP_INV;
    }

    return accum;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;
    float4 scene = gTexture.Sample(gSampler, uv);

    float height = 1.0f - uv.y;
    float ns = saturate(NoiseStrength);

    // ----------------------------
    // 1. ベース霧（高さで軽く変化）
    // ----------------------------
    float heightMask = 0.0f;
    if (FogEnd > FogStart + 1e-5f)
    {
        heightMask = saturate((height - FogStart) / (FogEnd - FogStart));
    }

    float heightFactor = lerp(0.8f, 1.25f, heightMask);
    float baseFog = FogDensity * heightFactor;

    // ----------------------------
    // 2. 空間固定っぽいレイヤー霧
    // ----------------------------
    float layerFog = integrateFogLayers(uv, height, ns, Time); // 0〜1

    float patch = (layerFog - 0.5f) * 2.0f; // -1〜+1
    patch *= ns;

    float fogRaw = baseFog * (1.0f + patch);

    float minFog = FogDensity * 0.20f;
    fogRaw = max(fogRaw, minFog);
    fogRaw = max(fogRaw, 0.0f);

    // ----------------------------
    // 3. カーブ
    // ----------------------------
    fogRaw = min(fogRaw, 2.0f);
    float fogAmount = saturate(1.0f - exp(-fogRaw));

    // ----------------------------
    // 4. 合成
    // ----------------------------
    float3 fogColor = FogColor;

    float sceneLuma = dot(scene.rgb, float3(0.299f, 0.587f, 0.114f));
    float fogLuma = dot(fogColor, float3(0.299f, 0.587f, 0.114f));
    float lightBoost = saturate(sceneLuma * 0.7f + fogLuma * 0.3f);
    fogColor *= lerp(0.9f, 1.2f, lightBoost);

    float3 fogged = lerp(scene.rgb, fogColor, fogAmount);

    output.color = float4(fogged, scene.a);
    return output;
}
