cbuffer Transform : register(b0)
{
    matrix viewProjection;
    matrix world;
};

struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 pos = mul(float4(input.position, 1), world);
    output.position = mul(pos, viewProjection).xyww; // ここ!!
    output.texcoord = input.position;
    return output;
}
