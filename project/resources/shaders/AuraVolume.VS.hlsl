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

struct VSIn
{
    float3 pos : POSITION; // unit quad: x=-1..1, y=0..1, z=0
    float2 uv : TEXCOORD0;
};

struct VSOut
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float2 local : TEXCOORD1; // (x,y) in unit quad
    float slice : TEXCOORD2; // 0..1
};

VSOut main(VSIn input, uint iid : SV_InstanceID)
{
    VSOut o;

    uint sc = max(SliceCount, 1);
    float t = (float) iid / (float) sc; // 0..(sc-1)/sc
    float ang = t * 6.2831853; // 2PI
    float s = sin(ang);
    float c = cos(ang);

    // quad local -> cylinder slice in world
    float3 p;
    p.x = input.pos.x * Radius;
    p.y = input.pos.y * Height;
    p.z = 0.0;

    // rotate around Y
    float3 rot;
    rot.x = p.x * c + p.z * s;
    rot.y = p.y;
    rot.z = -p.x * s + p.z * c;

    float3 world = CenterWS + rot;

    // あなたの行列はCPU側で View*Proj を作ってるので、こっちは右掛けで統一
    o.svpos = mul(float4(world, 1.0), ViewProj);
    o.uv = input.uv;
    o.local = input.pos.xy;
    o.slice = t;

    return o;
}
