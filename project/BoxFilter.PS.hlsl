#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

static const float2 kIndex3x3[3][3] =
{
    { float2(-1.0f, -1.0f), float2(0.0f, -1.0f), float2(1.0f, -1.0f) },
    { float2(-1.0f, 0.0f), float2(0.0f, 0.0f), float2(1.0f, 0.0f) },
    { float2(-1.0f, 1.0f), float2(0.0f, 1.0f), float2(1.0f, 1.0f) }
};

static const float kKernel3x3[3][3] =
{
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f }
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStepSize = float2(1.0f / width, 1.0f / height);

    float3 result = float3(0.0f, 0.0f, 0.0f);

    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            float2 offsetUV = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            float3 texColor = gTexture.Sample(gSampler, offsetUV).rgb;
            result += texColor * kKernel3x3[x][y];
        }
    }

    output.color = float4(result, 1.0f);
    return output;
}
