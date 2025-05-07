#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// CBuffer で渡すと良い定数たち
static const float2 kCenter = float2(0.5f, 0.5f); // ブラーの中心
static const float kBlurWidth = 0.01f; // ブラーの幅
static const int kNumSamples = 10; // サンプル数（多いほど滑らか）

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    float3 outputColor = float3(0.0f, 0.0f, 0.0f);
    float2 direction = input.texcoord - kCenter;

    for (int sampleIndex = 0; sampleIndex < kNumSamples; ++sampleIndex)
    {
        float2 texcoord = input.texcoord - direction * kBlurWidth * float(sampleIndex);
        outputColor += gTexture.Sample(gSampler, texcoord).rgb;
    }

    // 平均化する
    outputColor *= rcp(kNumSamples);

    PixelShaderOutput output;
    output.color.rgb = outputColor;
    output.color.a = 1.0f;
    return output;
}
