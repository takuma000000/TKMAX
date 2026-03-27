Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer NoiseCB : register(b0)
{
    float gTime;
    float gIntensity;
    float gLineDensity;
    float gLineSpeed;

    float gBlockScale;
    float gBlockShift;
    float gRGBShift;
    float gFlash;

    float2 gResolution;
    float gPad0;
    float gPad1;
};

struct PSInput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float Hash11(float p)
{
    p = frac(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return frac(p);
}

float3 RGBToHSV(float3 c)
{
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 HSVToRGB(float3 c)
{
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

float Hash21(float2 p)
{
    float3 p3 = frac(float3(p.x, p.y, p.x) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.uv;
    float2 glitchUV = uv;

    //=========================================================
    // 1. 横線単位のズレ
    //=========================================================
    float lineId = floor(uv.y * gLineDensity + gTime * gLineSpeed);
    float lineNoise = Hash11(lineId);
    float lineMask = step(0.72, lineNoise); // 一部の線だけ崩す
    float lineShift = (lineNoise * 2.0 - 1.0) * 0.08 * gIntensity * lineMask;
    glitchUV.x += lineShift;

    //=========================================================
    // 2. ブロック単位のズレ
    //=========================================================
    float2 blockUV = floor(uv * gBlockScale) / gBlockScale;
    float blockNoise = Hash21(blockUV + floor(gTime * 7.0));
    float blockMask = step(0.6, blockNoise);
    float2 blockShift = float2(
        (Hash21(blockUV + 1.37) * 2.0 - 1.0) * gBlockShift * gIntensity,
        (Hash21(blockUV + 8.91) * 2.0 - 1.0) * gBlockShift * 0.35 * gIntensity
    );
    glitchUV += blockShift * blockMask;

    //=========================================================
    // 3. RGBずれ
    //=========================================================
    float rgbJitter = (Hash11(floor(gTime * 60.0)) * 2.0 - 1.0) * gRGBShift * gIntensity;

    float r = gTexture.Sample(gSampler, glitchUV + float2(rgbJitter, 0.0)).r;
    float g = gTexture.Sample(gSampler, glitchUV).g;
    float b = gTexture.Sample(gSampler, glitchUV - float2(rgbJitter, 0.0)).b;

    float3 color = float3(r, g, b);
    
    //=========================================================
    // 4. 色バグブロック
    // 派手に色を乗せず、局所的に少しだけ色を壊す
    //=========================================================
    float2 colorBlockUV = floor(uv * (gBlockScale * 0.95)) / (gBlockScale * 0.95);
    float colorBlockNoise = Hash21(colorBlockUV + floor(gTime * 3.0) * 1.17);
    float colorBlockMask = step(0.90, colorBlockNoise) * gIntensity;

    if (colorBlockMask > 0.0)
    {
        float3 hsv = RGBToHSV(color);

    // 色相はかなり弱く、ほんの少しだけズラす
        float hueShift = (Hash21(colorBlockUV + 3.21) * 2.0 - 1.0) * 0.015;
        hsv.x = frac(hsv.x + hueShift);

    // 彩度は上げず、少し抜ける方向に寄せる
        float satMul = lerp(0.90, 1.00, Hash21(colorBlockUV + 7.13));
        hsv.y = saturate(hsv.y * satMul);

    // 明るさは上げず、少しだけ沈む方向を中心にする
        float valueMul = lerp(0.88, 1.00, Hash21(colorBlockUV + 9.41));
        hsv.z = saturate(hsv.z * valueMul);

        color = HSVToRGB(hsv);

    // ごく一部だけ、わずかに色チャンネルのバランスを崩す
        float channelDrift = step(0.95, Hash21(colorBlockUV + 12.77));
        if (channelDrift > 0.0)
        {
            color.r *= 0.97;
            color.g *= 1.01;
            color.b *= 1.02;
        }
    }

    //=========================================================
    // 5. 走査線っぽい明滅
    //=========================================================
    float scan = sin(uv.y * gResolution.y * 0.35 + gTime * 55.0) * 0.5 + 0.5;
    color *= lerp(0.92, 1.08, scan * 0.35 * gIntensity);

    //=========================================================
    // 6. 白飛びっぽいフラッシュ
    //=========================================================
    float flashNoise = Hash11(floor(gTime * 24.0));
    float flashMask = step(0.86, flashNoise);
    color += flashMask * gFlash * gIntensity;

    return float4(saturate(color), 1.0f);
}