#include "CopyImage.hlsli"

// 入力テクスチャとサンプラー
Texture2D<float4> gSceneTex : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// ----------------------------------------
// 疑似乱数: 2D 座標 → 0〜1
// ----------------------------------------
float rand2dTo1d(float2 uv)
{
    float2 k = float2(12.9898, 78.233);
    float r = sin(dot(uv, k)) * 43758.5453;
    return frac(r);
}

// メイン
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;

    // 元のシーンカラー
    float4 sceneColor = gSceneTex.Sample(gSampler, uv);

    // ------------------------------
    // ★ ノイズパラメータ（固定値）
    //   ※あとで ImGui 連携したくなったら、
    //     そのとき CBV 方式に差し替えればOK
    // ------------------------------
    static const float NoiseStrength = 0.4f; // 0〜1

    // uv ベースでノイズ値を生成（0〜1）
    float noise = rand2dTo1d(uv * 30.0f);

    float3 noiseColor = float3(noise, noise, noise);

    // シーン色にノイズを乗算（強さ付き）
    float3 mixed = lerp(sceneColor.rgb, sceneColor.rgb * noiseColor, NoiseStrength);

    output.color.rgb = mixed;
    output.color.a = sceneColor.a;
    return output;
}
