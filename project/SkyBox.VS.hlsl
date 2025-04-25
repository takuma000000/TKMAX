#include "SkyBox.hlsli"

struct TransformationMatrix
{
    float4x4 wvp;
    float4x4 World;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    output.position = mul(input.position, gTransformationMatrix.wvp).xyww;

    // キューブマップ用にワールド方向ベクトルを使う（正規化）
    output.texcoord = input.position.xyz;

    return output;
}