#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gMaskTexture : register(t1); // ノイズマスク
SamplerState gSampler : register(s0);

cbuffer ThresholdBuffer : register(b0)
{
    float threshold;
    float padding[3]; // 16byte アライメント
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float mask = gMaskTexture.Sample(gSampler, input.texcoord);

    if (mask < threshold)
    {
        discard;
    }

    output.color = gTexture.Sample(gSampler, input.texcoord);
    return output;
}
