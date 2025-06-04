#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

static const float kPrewittHorizontalKernel[3][3] =
{
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f }
};

static const float kPrewittVerticalKernel[3][3] =
{
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f }
};

static const float2 kIndex3x3[3][3] =
{
    { float2(-1, -1), float2(0, -1), float2(1, -1) },
    { float2(-1, 0), float2(0, 0), float2(1, 0) },
    { float2(-1, 1), float2(0, 1), float2(1, 1) }
};

float Luminance(float3 color)
{
    return dot(color, float3(0.2125f, 0.7154f, 0.0721f));
}

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStep = float2(1.0f / width, 1.0f / height);

    float2 diff = float2(0.0f, 0.0f);
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            float2 offset = input.texcoord + kIndex3x3[x][y] * uvStep;
            float3 color = gTexture.Sample(gSampler, offset).rgb;
            float lum = Luminance(color);
            diff.x += lum * kPrewittHorizontalKernel[x][y];
            diff.y += lum * kPrewittVerticalKernel[x][y];
        }
    }

    float weight = length(diff);
    weight = saturate(weight * 6.0f); // スライドの合成用に強調倍率

    float3 baseColor = gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.rgb = (1.0f - weight) * baseColor; // 輪郭部分を黒に近づける合成
    output.color.a = 1.0f;

    return output;
}
