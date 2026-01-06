struct PSIn
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float beamT : TEXCOORD1;
};

cbuffer LaserBeamCB : register(b0)
{
    float4x4 ViewProj;

    float3 StartWS;
    float Radius;

    float3 EndWS;
    float Intensity;

    float3 CamRightWS;
    float _pad0;
    float3 CamUpWS;
    float _pad1;
    float3 CamFwdWS;
    float _pad2;

    uint SliceCount;
    float Time;
    float CoreSharpness;
    float EdgeSoftness;

    float3 Color;
    float NoiseScale;

    float NoiseSpeed;
    uint Telegraph;
    float _pad3;
    float _pad4;
};

float hash11(float p)
{
    p = frac(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return frac(p);
}

// 0..1
float noise1(float x)
{
    float i = floor(x);
    float f = frac(x);
    float a = hash11(i);
    float b = hash11(i + 1.0);
    float u = f * f * (3.0 - 2.0 * f);
    return lerp(a, b, u);
}

float4 main(PSIn input) : SV_TARGET
{
    // quad local [-1..1]
    float2 q = input.uv * 2.0f - 1.0f;
    float r = length(q);

    // 中心コア（締まった光）
    float core = exp(-r * r * max(CoreSharpness, 0.1f));

    // 外側（ふわっと）
    float edge = exp(-r * r * (1.0f / max(EdgeSoftness, 0.001f)) * 3.0f);

    // ビーム方向のゆらぎ（ちらつき）
    float t = Time * NoiseSpeed;
    float n = noise1(input.beamT * max(NoiseScale, 0.001f) * 7.0f + t);
    n = (n - 0.5f) * 2.0f; // -1..1

    float flicker = 1.0f + n * 0.18f;

    // 予告：点滅（弱め）
    if (Telegraph != 0)
    {
        float pulse = 0.35f + 0.65f * abs(sin(Time * 10.5f));
        flicker *= pulse;
    }

    // アルファ（加算用）
    float a = (core * 1.0f + edge * 0.45f) * Intensity * flicker;

    // 端を切る（板が見えないように）
    // r=1より外は消える
    float mask = saturate(1.0f - smoothstep(0.92f, 1.0f, r));
    a *= mask;

    // 明るすぎ防止
    a = saturate(a);

    // 加算ブレンド想定：色はそのまま返す（αは強度）
    return float4(Color * a, a);
}