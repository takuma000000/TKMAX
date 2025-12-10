#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// C++側の DirectXCommon::WaterRippleCB とレイアウトを合わせる
cbuffer RippleCB : register(b0)
{
    float2 Center; // 波紋中心 (UV)  = center
    float Radius; // 現在の半径    = radius
    float Amplitude; // ズレの強さ    = amplitude
    float Frequency; // 波の細かさ    = frequency
    float Width; // 帯の幅        = width
    float Padding; // アライメント  = padding
    float3 RippleColor; // 波紋色        = color
    float ColorIntensity; // 色の強さ      = colorIntensity
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;

    // 元のシーンカラー
    float4 sceneColor = gTexture.Sample(gSampler, uv);

    // 波紋が無効 or 半径が 0 ならそのまま返す
    if (Radius <= 0.0f || Amplitude == 0.0f)
    {
        output.color = sceneColor;
        return output;
    }

    // 中心からのベクトルと距離
    float2 dir = uv - Center;
    float dist = length(dir);

    // 方向（正規化） 距離がめちゃ小さい時は 0 にしておく
    float2 dirN = (dist > 1e-4f) ? dir / dist : float2(0.0f, 0.0f);

    // --------------------------
    // 波紋の「輪」のマスクを作る
    //   dist が Radius 付近だけ 1 に近くなるような値
    // --------------------------
    float band = abs(dist - Radius) * Width; // 離れるほど大きく
    float mask = saturate(1.0f - band); // 0〜1 にクランプ

    // 波の揺れ
    float wave = sin(dist * Frequency);
    float offset = wave * Amplitude * mask;

    // UV をずらして再サンプリング
    float2 rippleUV = uv + offset * dirN;
    float4 rippleColorTex = gTexture.Sample(gSampler, rippleUV);

    // --------------------------
    // 色を乗せる
    // --------------------------
    // mask を少し強めたいなら pow(mask, 何か) とかでもOK
    float colorMask = mask * ColorIntensity;

    // 元のテクスチャ色に RippleColor をブレンド
    float3 finalRgb = lerp(rippleColorTex.rgb, RippleColor, saturate(colorMask));

    output.color.rgb = finalRgb;
    output.color.a = rippleColorTex.a; // アルファは元のまま

    return output;
}
