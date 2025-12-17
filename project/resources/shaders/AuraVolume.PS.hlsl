cbuffer AuraVolumeCB : register(b0)
{
    float4x4 ViewProj;

    float3 CenterWS;
    float Radius;

    float Height;
    uint SliceCount;
    float Time;
    float _pad0;

    float3 Color;
    float Intensity;

    float NoiseScale;
    float NoiseSpeed;
    float RimPower;
    float AlphaBase;
};

struct PSIn
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float2 local : TEXCOORD1;
    float slice : TEXCOORD2;
};

float hash21(float2 p)
{
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}

float4 main(PSIn i) : SV_TARGET
{
    // local.x : -1..1, local.y : 0..1
    float x = abs(i.local.x);
    float y = saturate(i.local.y);

    // 板の中心ほど濃い（体積感）
    float core = exp(-x * x * 2.2);

    // 上下フェード（足元・頭で切れない）
    float vfade = smoothstep(0.0, 0.10, y) * smoothstep(0.0, 0.20, 1.0 - y);

    // 外周リム（包む輪郭）
    float rim = pow(saturate(1.0 - x), RimPower);

    // 炎っぽいゆらぎ（縦+スライス+時間）
    float w1 = sin((y * NoiseScale) + (i.slice * 6.0) + Time * NoiseSpeed) * 0.5 + 0.5;
    float w2 = sin((y * (NoiseScale * 1.7)) - (i.slice * 9.0) + Time * (NoiseSpeed * 1.6)) * 0.5 + 0.5;
    float n = (w1 * 0.7 + w2 * 0.3);

    float rnd = hash21(float2(i.slice * 13.7, floor(y * 18.0 + Time * NoiseSpeed * 6.0)));
    float streak = smoothstep(0.55, 1.0, rnd);

    float flame = (n * 0.75 + streak * 0.55) * vfade;

    float a = (core * 0.75 + rim * flame) * AlphaBase * Intensity;

    // 加算想定：RGBはColor*a、αはブレンド係数
    return float4(Color * a, a);
}
