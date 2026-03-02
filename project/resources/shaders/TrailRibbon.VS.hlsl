cbuffer CB : register(b0)
{
    float4x4 gViewProj;
    float gTime;
    float gUvScroll;
    float gIntensity;
    float gPad0;
};

struct VSIn
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float age01 : TEXCOORD1;
};

struct VSOut
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float age01 : TEXCOORD1;
};

VSOut main(VSIn i)
{
    VSOut o;
    o.svpos = mul(float4(i.pos, 1.0f), gViewProj);

	// uv.x にスクロールを加える（流れる線）
    o.uv = i.uv;
    o.uv.x += gTime * gUvScroll;

    o.color = i.color;
    o.age01 = i.age01;
    return o;
}