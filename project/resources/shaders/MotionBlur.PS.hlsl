Texture2D<float4> gCurrentTexture : register(t0);
Texture2D<float4> gPreviousTexture : register(t1);
SamplerState gSampler : register(s0);

cbuffer MotionBlurParam : register(b0)
{
    float strength;
    float3 padding;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    float4 currentColor = gCurrentTexture.Sample(gSampler, input.texcoord);
    float4 previousColor = gPreviousTexture.Sample(gSampler, input.texcoord);

    return lerp(currentColor, previousColor, strength);
}