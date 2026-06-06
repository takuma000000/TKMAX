Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer SpeedLineCB : register(b0)
{
    float2 gDirection;
    float gIntensity;
    float gTime;

    float gDensity;
    float gSpeed;
    float gWidth;
    float gPadding;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float Random(float x)
{
    return frac(sin(x * 127.1f) * 43758.5453f);
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    float2 uv = input.texcoord;
    float4 baseColor = gTexture.Sample(gSampler, uv);

    //=========================================================
    // 中心座標
    //=========================================================

    float2 center = float2(0.5f, 0.5f);

    float2 v = uv - center;

    // 16:9補正
    v.x *= 16.0f / 9.0f;

    float radius = length(v);
    float angle = atan2(v.y, v.x);

    //=========================================================
    // 線の太さ
    //=========================================================

    float widthScale =
        lerp(
            0.25f,
            6.0f,
            saturate(radius)
        );

    //=========================================================
    // 放射状ライン生成
    //=========================================================

    float angleIndex =
        floor((angle + 3.14159265f) * gDensity);

    float randomWidth =
        lerp(
            0.25f,
            2.8f,
            Random(angleIndex)
        );

    float randomAlpha =
        lerp(
            0.55f,
            1.0f,
            Random(angleIndex + 12.34f)
        );

    float pattern =
        frac((angle + 3.14159265f) * gDensity);

    float distToLine =
        abs(pattern - 0.5f);

    float lineMask =
        smoothstep(
            gWidth * randomWidth * widthScale,
            0.0f,
            distToLine
        );

    //=========================================================
    // マスク
    //=========================================================

    // 中心近くまで線を伸ばす
    float outerMask =
        smoothstep(
            0.001f,
            0.18f,
            radius
        );

    // 外周を少し強調
    float edgeMask =
        smoothstep(
            0.05f,
            0.85f,
            radius
        );

    //=========================================================
    // 明滅
    //=========================================================

    float flicker =
        lerp(
            0.85f,
            1.20f,
            Random(angleIndex + floor(gTime * gSpeed))
        );

    float alpha =
        lineMask *
        outerMask *
        edgeMask *
        randomAlpha *
        flicker *
        gIntensity;

    //=========================================================
    // 色
    //=========================================================

    float3 lineColor =
        float3(
            0.92f,
            0.98f,
            1.0f
        );

    // 線を目立たせる
    baseColor.rgb =
        lerp(
            baseColor.rgb,
            lineColor,
            saturate(alpha * 1.45f)
        );

    return baseColor;

}