#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSamplerLinear : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    // 中心点。ここを基準に放射状にブラーがかかる
    const float2 kCenter = float2(0.5f, 0.5f);
    // サンプリング数。多いほど滑らかだが重い
    const int kNumSamples = 10;
    // ぼかしの幅。大きいほど広くぼける
    const float kBlurWidth = 0.01f;

    // 中心から現在の uv に対しての方向を計算。
    // 単位ベクトルではないが、ここではあえて正規化せず、遠いほどより遠くをサンプリングする
    float2 direction = input.texcoord - kCenter;
    float3 outputColor = float3(0.0f, 0.0f, 0.0f);

    for (int sampleIndex = 0; sampleIndex < kNumSamples; ++sampleIndex)
    {
        // 現在の uv からさほどに沿って計算した方向にサンプリング点を進めながらサンプリングしていく
        float2 texcoord = input.texcoord + direction * kBlurWidth * float(sampleIndex);
        outputColor.rgb += gTexture.Sample(gSamplerLinear, texcoord).rgb;
    }

    // 平均化する
    outputColor.rgb *= rcp(float(kNumSamples));

    PixelShaderOutput output;
    output.color.rgb = outputColor;
    output.color.a = 1.0f;
    return output;
}
