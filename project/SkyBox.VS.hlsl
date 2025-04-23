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
    float4 worldPos = mul(input.position, gTransformationMatrix.World);
    output.position = mul(worldPos, gTransformationMatrix.wvp);

    // キューブマップ用にワールド方向ベクトルを使う（正規化）
    output.texcoord = normalize(worldPos.xyz);

    return output;
}