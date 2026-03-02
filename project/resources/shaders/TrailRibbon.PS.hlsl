cbuffer CB : register(b0)
{
    float4x4 gViewProj;
    float gTime;
    float gUvScroll;
    float gIntensity;
    float gPad0;
};

struct PSIn
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float age01 : TEXCOORD1;
};

// テクスチャ無しの「パンツァっぽい」発光：
// - V方向で中心が明るく端が落ちる
// - U方向で少し流れる模様（sin）
float4 main(PSIn i) : SV_TARGET
{
	// ribbon の縁フェード（v=0..1）
    float v = i.uv.y;
    float edge = 1.0f - abs(v * 2.0f - 1.0f); // 1 at center
    edge = saturate(edge);
    edge = pow(edge, 1.8f);

	// tail側を薄く（age01=0 head, 1 tail）
    float tailFade = saturate(1.0f - i.age01);
    tailFade = pow(tailFade, 1.4f);

	// 流れる模様（U方向）
    float flow = 0.75f + 0.25f * sin(i.uv.x * 12.0f);

    float a = edge * tailFade;
    float3 emissive = i.color.rgb * (gIntensity * flow) * a;

	// 加算前提：alphaは使わないが一応入れる
    return float4(emissive, a);
}