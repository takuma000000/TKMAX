struct VSIn
{
    float3 pos : POSITION; // [-1..1] quad
    float2 uv : TEXCOORD;
};

struct VSOut
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float beamT : TEXCOORD1; // 0..1 (along beam)
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
    uint Telegraph; // 1=予告
    float _pad3;
    float _pad4;
};

VSOut main(VSIn input, uint instanceId : SV_InstanceID)
{
    VSOut o;

    uint sc = max(SliceCount, 1);
    float t = (sc == 1) ? 0.5f : (instanceId / (float) (sc - 1));
    o.beamT = t;

    float3 center = lerp(StartWS, EndWS, t);

    // quad coords
    float2 q = input.pos.xy; // [-1..1]

    // カメラ基底でビルボード（Fogと同じ考え方）
    float3 ws =
        center
        + CamRightWS * (q.x * Radius)
        + CamUpWS * (q.y * Radius);

    o.uv = input.uv;
    o.svpos = mul(float4(ws, 1.0f), ViewProj);
    return o;
}