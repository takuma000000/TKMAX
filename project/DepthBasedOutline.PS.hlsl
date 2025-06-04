#include "CopyImage.hlsli"

// 逆射影行列を受け取る（Projection行列のInverse）
cbuffer Material : register(b0)
{
    float4x4 projectionInverse;
};

// Depthテクスチャ（float1）
Texture2D<float> gDepth : register(t0);
// PointSampling用Sampler
SamplerState gSampler : register(s0);

// Prewittカーネル
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

// サンプル位置（3x3）
static const float2 kOffset[3][3] =
{
    { float2(-1, -1), float2(0, -1), float2(1, -1) },
    { float2(-1, 0), float2(0, 0), float2(1, 0) },
    { float2(-1, 1), float2(0, 1), float2(1, 1) }
};

float4 main(VertexShaderOutput input) : SV_TARGET
{
    uint width, height;
    gDepth.GetDimensions(width, height);
    float2 texelSize = float2(1.0f / width, 1.0f / height);

    float2 diff = float2(0.0f, 0.0f);

    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            // テクスチャ座標をオフセット
            float2 uv = input.texcoord + kOffset[y][x] * texelSize;

            // 深度（NDC空間Z）を取得
            float ndcDepth = gDepth.Sample(gSampler, uv);

            // NDC空間のXYは [0~1] -> [-1~1] に
            float2 ndcXY = uv * 2.0f - 1.0f;
            float4 ndcPos = float4(ndcXY, ndcDepth, 1.0f);

            // 逆射影してview空間に変換
            float4 viewPos = mul(ndcPos, projectionInverse);
            viewPos /= viewPos.w;

            float z = viewPos.z;

            // Prewittフィルタ適用
            diff.x += z * kPrewittHorizontalKernel[y][x];
            diff.y += z * kPrewittVerticalKernel[y][x];
        }
    }

    float weight = length(diff);

    // スライドでは輝度で4倍にしていた → 同じように強調率を調整
    weight = saturate(weight * 4.0f);

    return float4(weight.xxx, 1.0f); // 輪郭のみ表示
}
