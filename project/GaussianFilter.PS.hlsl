#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

static const float2 kIndex3x3[3][3] =
{
    { float2(-1.0f, -1.0f), float2(0.0f, -1.0f), float2(1.0f, -1.0f) },
    { float2(-1.0f, 0.0f), float2(0.0f, 0.0f), float2(1.0f, 0.0f) },
    { float2(-1.0f, 1.0f), float2(0.0f, 1.0f), float2(1.0f, 1.0f) }
};

static const float PI = 3.14159265f;

// ガウス関数の定義
float gauss(float x, float y, float sigma)
{
    float exponent = -((x * x + y * y) / (2.0f * sigma * sigma));
    float denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) / denominator;
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
    float2 uvStepSize = float2(1.0f / width, 1.0f / height);

    float3 result = float3(0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;

    float sigma = 2.0f; // 標準偏差（適宜 ImGui や CBuffer で変更可）

    // 3x3 ガウシアンフィルタ
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            float2 offset = kIndex3x3[x][y];
            float2 offsetUV = input.texcoord + offset * uvStepSize;

            float weight = gauss(offset.x, offset.y, sigma);
            float3 texColor = gTexture.Sample(gSampler, offsetUV).rgb;

            result += texColor * weight;
            weightSum += weight;
        }
    }

    result /= weightSum;
    output.color = float4(result, 1.0f);
    return output;
}
