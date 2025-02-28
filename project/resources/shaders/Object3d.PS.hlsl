#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess; // 追加
};

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct Camera
{
    float3 worldPosition;
};

ConstantBuffer<Camera> gCamera : register(b2);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    //Materialを拡張する
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);

    // テクスチャサンプルを行う
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // ピクセルの色を計算する
    //output.color = gMaterial.color * textureColor;

    if (gMaterial.enableLighting != 0)
    {
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    
        float3 toEye = normalize(gCamera.worldPosition - input.worldPosition); // カメラ方向
        float3 reflectLight = reflect(gDirectionalLight.direction, normalize(input.normal)); // 反射光

        float3 halfVector = normalize(-gDirectionalLight.direction + toEye); // ハーフベクトル
        float NDotH = dot(normalize(input.normal), halfVector);
        
        float specularPow = pow(saturate(NDotH), gMaterial.shininess); // スペキュラー

        float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity; // 拡散反射
    
        float3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);
    
        output.color.rgb = diffuse + specular; // 合計色
        output.color.a = gMaterial.color.a * textureColor.a; // 透明度
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }

    return output;
}