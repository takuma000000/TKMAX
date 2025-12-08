#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// CPU から送るビネット用パラメータ
cbuffer VignettingCB : register(b0)
{
    float4 VignetteColor; // 枠の色 (rgb) / a は未使用
    float VignetteIntensity; // 強度 (0〜1〜2 くらい)
    float VignetteRadius; // どこから暗くし始めるか (0〜1)
    float VignetteSoftness; // ふちのボケ具合 (0〜1)
    float _padding; // 16byte アライメント用
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float Remap01(float v, float a, float b)
{
    return saturate((v - a) / (b - a));
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;

    // 元のシーンカラー
    float4 sceneColor = gTexture.Sample(gSampler, uv);

    // 画面中心 (0.5,0.5) からの距離
    float2 centered = uv - float2(0.5f, 0.5f);
    // 四隅あたりで 1 くらいになるよう √2 を掛ける
    float dist = length(centered) * 1.41421356f;

    // inner = ほぼ無影響な内側, outer = 最大ビネットになる外側
    float inner = VignetteRadius;
    float outer = saturate(VignetteRadius + VignetteSoftness + 1e-5f);

    // t = 0 → 中央, 1 → 外側
    float t = Remap01(dist, inner, outer);

    // Intensity を 0〜1 にクランプ
    float intensity = saturate(VignetteIntensity);

    // 1. 輝度を暗くする係数
    float brightness = lerp(1.0f, 1.0f - intensity, t);

    // 2. 枠色をブレンド（外側ほど VignetteColor に寄る）
    float3 tinted = lerp(sceneColor.rgb, VignetteColor.rgb, t * intensity);

    output.color.rgb = tinted * brightness;
    output.color.a = sceneColor.a;
    return output;
}
