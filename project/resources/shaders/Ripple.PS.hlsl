#include "CopyImage.hlsli"

// RenderTexture（画面）のテクスチャとサンプラ
Texture2D<float4> gSceneTex : register(t0);
SamplerState gSampler : register(s0);

// 波紋用の定数バッファ
cbuffer RippleCB : register(b0)
{
    float2 center; // 波紋の中心 (UV 空間 0〜1)
    float radius; // 現在の半径
    float amplitude; // ズレ量の強さ
    float frequency; // 波の細かさ
    float width; // 帯の幅(減衰の鋭さ)
    float _pad; // 16byte アライメント用
};

// VS から渡ってくる頂点
float4 main(VertexShaderOutput input) : SV_TARGET
{
    float2 uv = input.texcoord;

    float2 dir = uv - center;
    float dist = length(dir);

    // 中心ど真ん中はそのまま
    if (dist < 1e-4)
    {
        return gSceneTex.Sample(gSampler, uv);
    }

    float2 n = dir / dist;

    // 現在のリングからの距離
    float d = dist - radius;

    // 帯のエンベロープ（中心から離れると弱くなる）
    float envelope = exp(-abs(d) * width);

    // 正弦波で波紋
    float wave = sin(d * frequency);

    // 実際のオフセット量
    float offset = wave * envelope * amplitude;

    float2 uvRipple = uv + n * offset;

    float4 baseCol = gSceneTex.Sample(gSampler, uv);
    float4 rippleCol = gSceneTex.Sample(gSampler, uvRipple);

    // 帯付近だけ強く混ぜる
    float rippleMask = saturate(envelope * 2.0f);

    return lerp(baseCol, rippleCol, rippleMask);
}
