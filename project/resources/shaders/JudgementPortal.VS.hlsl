struct VSInput
{
    float3 pos : POSITION0;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

cbuffer JudgementPortalCB : register(b0)
{
    float4x4 gViewProj;

    float3 gCenterWS;
    float gTime;

    float3 gCamRight;
    float gSize;

    float3 gCamUp;
    float gCharge01;

    float3 gCamFwd;
    float gIntensity;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    float3 worldPos =
		gCenterWS +
		gCamRight * input.pos.x * gSize * 0.5f +
		gCamUp * input.pos.y * gSize * 0.5f;

    output.worldPos = worldPos;
    output.pos = mul(float4(worldPos, 1.0f), gViewProj);
    output.uv = input.uv;

    return output;
}