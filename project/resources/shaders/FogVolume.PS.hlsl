struct PSIn
{
    float4 svpos : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float sliceT : TEXCOORD2;
};

cbuffer FogVolumeCB : register(b0)
{
    float4x4 ViewProj;

    float3 CenterWS;
    float _pad0;
    float3 HalfSizeWS;
    float Density;

    float3 CamRightWS;
    float _pad1;
    float3 CamUpWS;
    float _pad2;
    float3 CamFwdWS;
    float _pad3;

    uint SliceCount;
    float Time;
    float NoiseScale;
    float NoiseSpeed;

    float3 FogColor;
    float Softness;

    float FogStart;
    float FogEnd;

    float NoiseStrength;
    float WorldScale;

    float3 WorldPos;
    float _padX;
};

// ----------------------------
// Fog.PS 風の 2D noise
// ----------------------------
float hash21(float2 p)
{
    p = frac(p * 0.3183099 + float2(0.71, 0.113));
    p *= 17.0;
    return frac(p.x * p.y * (p.x + p.y));
}

float smoothNoise2D(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    float a = hash21(i);
    float b = hash21(i + float2(1.0, 0.0));
    float c = hash21(i + float2(0.0, 1.0));
    float d = hash21(i + float2(1.0, 1.0));

    float2 u = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

float fbm2(float2 p)
{
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; i++)
    {
        v += a * smoothNoise2D(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v; // 0..1 くらい
}

// 箱の端フェード（いままで通り）
float boxFade(float3 p, float3 center, float3 halfSize, float softness)
{
    float3 q = abs((p - center) / max(halfSize, 1e-3));
    float d = max(q.x, max(q.y, q.z)); // 0=中心, 1=箱の面
    float a = saturate(1.0 - d);
    return pow(a, max(softness, 0.001));
}

float4 main(PSIn input) : SV_TARGET
{
    // 端フェード（体積感の核）
    float edge = boxFade(input.worldPos, CenterWS, HalfSizeWS, Softness);

    // 体積の高さを 0..1 に正規化（下ほど濃くしたいので反転）
    float y0 = CenterWS.y - HalfSizeWS.y;
    float y1 = CenterWS.y + HalfSizeWS.y;
    float h = (y1 > y0 + 1e-5f) ? saturate((input.worldPos.y - y0) / (y1 - y0)) : 0.5f;
    float height = 1.0f - h; // 下=1 / 上=0

    // Fog.PS の FogStart/FogEnd っぽい高さマスク
    float heightMask = 0.0f;
    if (FogEnd > FogStart + 1e-5f)
    {
        heightMask = saturate((height - FogStart) / (FogEnd - FogStart));
    }

    // Fog.PS の WorldPos/WorldScale 系（ノイズが“張り付き版”と同じ感じになる）
    float ws = max(WorldScale, 1e-3f);
    float3 local = (input.worldPos - WorldPos) / ws;

    // 2Dノイズ（XZ）＋時間で流す
    float2 np = local.xz * NoiseScale;
    np += float2(Time * NoiseSpeed, -Time * NoiseSpeed * 0.7f);

    float n = fbm2(np); // 0..1
    float ns = saturate(NoiseStrength);

    // Fog.PS っぽく「ノイズで濃さが揺れる」方向
    float noiseTerm = (n - 0.5f) * 2.0f; // -1..1
    noiseTerm *= ns;

    // ベース濃度（体積・高さ・端フェード）
    float base = Density;
    base *= edge;
    base *= lerp(0.75f, 1.25f, heightMask);

    // Fog.PS の “expカーブ”
    float fogRaw = max(0.0f, base + noiseTerm * 0.25f);
    fogRaw = min(fogRaw, 2.0f);
    float alpha = saturate(1.0f - exp(-fogRaw));

    return float4(FogColor, alpha);
}
