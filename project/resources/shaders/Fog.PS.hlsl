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
// ハッシュ → value noise 用
// ----------------------------
float hash21(float2 p)
{
    p = frac(p * 0.3183099 + float2(0.71, 0.113));
    p *= 17.0;
    return frac(p.x * p.y * (p.x + p.y));
}

// 補間付き value noise (2D)
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

// fbm っぽく階層ノイズ
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

// ----------------------------
// 擬似 "ボリューム" 霧レイヤー
// 画面奥方向に何層も霧を積んで積分するイメージ
// ----------------------------
float integrateFogLayers(float2 uv, float height, float baseScale, float ns, float time)
{
    // サンプル数（奥行き方向の層の数）
    const int STEPS = 6;
    const float STEP_INV = 1.0f / STEPS;

    // 風向き（霧が流れる方向）
    float2 wind = normalize(float2(0.6f, 0.25f));

    // ノイズスケールのクランプ
    baseScale = lerp(0.8f, 3.0f, saturate(NoiseScale * 0.1f));

    float accum = 0.0f;

    [unroll]
    for (int i = 0; i < STEPS; ++i)
    {
        // 0〜1 の奥行きパラメータ
        float t = (i + 0.5f) * STEP_INV;

        // 奥側ほど少しスケールを変える
        float layerScale = baseScale * lerp(1.0f, 1.8f, t);

        // 高さによる変位（上に行くほど風の影響が変わる）
        float heightOffset = height * (0.5f + t);

        // 基本の座標：画面UV + 風 + 高さの寄与
        float2 coord =
            uv * layerScale
            + wind * (time * 0.05f + t * 3.0f)
            + float2(heightOffset * 0.8f, -heightOffset * 0.6f);

        // ちょっと蛇行させる
        coord += float2(
            sin(time * 0.3f + t * 4.1f),
            cos(time * 0.27f + t * 5.3f)
        ) * 0.15f;

        float n = fbm2D(coord); // 0〜1

        // 「塊」のみを抜き出す（弱いノイズは無視）
        float threshold = 0.45f;
        float softness = 0.18f;
        float patch = smoothstep(threshold,
                                 threshold + softness,
                                 n); // 0〜1

        // レイヤーごとに「呼吸」させる（時間で膨らんだりしぼんだり）
        float breatheNoise = fbm2D(coord * 0.5f + float2(time * 0.07f, -time * 0.05f));
        float breathe = lerp(0.7f, 1.4f, breatheNoise); // 0.7〜1.4
        patch *= breathe;

        // 高さが低いほど濃くなる（地面付近の方が霧が濃い）
        float heightWeight = saturate(1.0f - height * 0.8f);
        patch *= heightWeight;

        // ノイズ強度（NoiseStrength）が 0 なら均一、1 ならフル
        patch *= ns;

        // 奥行き方向のステップ長で積分
        accum += patch * STEP_INV;
    }

    return accum; // 大体 0〜1 に収まる想定
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;
    float4 scene = gTexture.Sample(gSampler, uv);

    // 画面下を 0、上を 1 とした高さ
    float height = 1.0f - uv.y;
    float ns = saturate(NoiseStrength); // 0〜1

    // ----------------------------
    // 1. 「画面全体」にかかるベースの霧
    // ----------------------------

    float heightMask = 0.0f;
    if (FogEnd > FogStart + 1e-5f)
    {
        heightMask = saturate((height - FogStart) / (FogEnd - FogStart));
    }
    // 下の方をちょっとだけ濃くする程度（0.8〜1.2）
    float heightFactor = lerp(0.8f, 1.2f, heightMask);

    // ベース霧：ほぼ全画面同じ濃さ
    float baseFog = FogDensity * heightFactor; // ここが「常にかかってる霧」

    // ----------------------------
    // 2. レイヤー霧で「濃さのムラ」と動き
    // ----------------------------

    float layerFog = integrateFogLayers(uv, height, NoiseScale, ns, Time); // 0〜1

    // 0〜1 → -1〜+1 の揺らぎに
    float patch = (layerFog - 0.5f) * 2.0f; // -1〜+1
    patch *= ns; // NoiseStrength でどれだけムラを付けるか

    // ベース霧に「増減」として乗せる
    float fogRaw = baseFog * (1.0f + patch);

    // 完全に消えてしまうのを防ぐため、下限を少し持たせる
    float minFog = FogDensity * 0.25f; // ベースの 25% は必ずかかる
    fogRaw = max(fogRaw, minFog);
    fogRaw = max(fogRaw, 0.0f);

    // ----------------------------
    // 3. 非線形カーブで霧っぽく
    // ----------------------------

    // 真っ白防止
    fogRaw = min(fogRaw, 2.0f);

    float fogAmount = saturate(1.0f - exp(-fogRaw));

    // ----------------------------
    // 4. 色のブレンド（少しだけ発光っぽい補正）
    // ----------------------------

    float3 fogColor = FogColor;

    float sceneLuma = dot(scene.rgb, float3(0.299f, 0.587f, 0.114f));
    float fogLuma = dot(fogColor, float3(0.299f, 0.587f, 0.114f));
    float lightBoost = saturate(sceneLuma * 0.7f + fogLuma * 0.3f);

    fogColor *= lerp(0.9f, 1.2f, lightBoost);

    float3 fogged = lerp(scene.rgb, fogColor, fogAmount);

    output.color.rgb = fogged;
    output.color.a = scene.a;
    return output;
}