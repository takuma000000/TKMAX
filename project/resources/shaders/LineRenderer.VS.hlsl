struct VSInput
{
    float3 pos : POSITION;
    float4 col : COLOR;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

// RootParameter(0) に viewProj を受け取る
cbuffer ViewProj : register(b0)
{
    float4x4 viewProj;
};

VSOutput main(VSInput input)
{
    VSOutput o;
    o.pos = mul(float4(input.pos, 1.0f), viewProj); // 行列を掛けてクリップ座標に
    o.col = input.col;
    return o;
}
