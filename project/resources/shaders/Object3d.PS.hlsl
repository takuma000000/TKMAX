#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
TextureCube<float4> gEnvironmentTexture : register(t1);

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess;
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

struct PointLight
{
    float4 color; //色
    float3 position; //位置
    float intensity; //強度
    float radius; //半径
    float decay; //減衰率
};

ConstantBuffer<PointLight> gPointLight : register(b3);

struct SpotLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float cosAngle;
    float cosFalloffStart;
};

ConstantBuffer<SpotLight> gSpotLight : register(b4);

struct Environment
{
    bool useEnvironment;
    float3 padding;
};

ConstantBuffer<Environment> environment : register(b5);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    if (textureColor.a == 0.0f)
    {
        discard;
    }

    if (gMaterial.enableLighting != 0)
    {
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        
        float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
        float3 reflectLight = reflect(gDirectionalLight.direction, normalize(input.normal));
        
        float3 halfVector = normalize(-gDirectionalLight.direction + toEye);
        float HDotH = dot(normalize(input.normal), halfVector);
        float specularPow = pow(saturate(HDotH), gMaterial.shininess);
        
        // 拡散反射
        float3 directionalLight_Diffuse =
        gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;

        // 鏡面反射
        float3 directionalLight_Specular =
        gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);

        //PointLight///////////////////////////////////////////////
        
        float distance = length(gPointLight.position - input.worldPosition); //距離の2乗
        float factor = pow(saturate(-distance / gPointLight.radius + 1.0f), gPointLight.decay);
        
        float3 pointLightDirection = normalize(input.worldPosition - gPointLight.position);
        
        float3 pointLight_HalfVector = normalize(-pointLightDirection + toEye);
        float pointLight_NDotH = dot(normalize(input.normal), pointLight_HalfVector);
        float pointLight_SpecularPow = pow(saturate(pointLight_NDotH), gMaterial.shininess);
         
        float pointLight_NDotL = dot(normalize(input.normal), -pointLightDirection);
         
        float pointLight_Cos = pow(pointLight_NDotL * 0.5 + 0.5f, 2.0f);
        
        float3 pointLight_Diffuse = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * pointLight_Cos * gPointLight.intensity * factor;
        
        float3 pointLight_Specular = gPointLight.color.rgb * gPointLight.intensity * pointLight_SpecularPow * factor * float3(1.0f, 1.0f, 1.0f);
        
        
        //SpotLight//////////////////////////////////////////////////
        
        float distance_Spot = length(gSpotLight.position - input.worldPosition); //距離の2乗
        float factor_Spot = pow(saturate(-distance_Spot / gSpotLight.distance + 1.0f), gSpotLight.decay);
        
        float3 spotLightDirectionOnSurFace = normalize(input.worldPosition - gSpotLight.position);
        
        float3 spotLight_HalfVector = normalize(-gSpotLight.direction + toEye);
        float spotLight_NDotH = dot(normalize(input.normal), spotLight_HalfVector);
        float spotLight_SpecularPow = pow(saturate(spotLight_NDotH), gMaterial.shininess);
        
        float spotLight_NDotL = dot(normalize(input.normal), -gSpotLight.direction);
        float spotLight_Cos = pow(spotLight_NDotL * 0.5 + 0.5f, 2.0f);
        
        float cosAngle = dot(spotLightDirectionOnSurFace, gSpotLight.direction);
        float falloffFactor = saturate((cosAngle - gSpotLight.cosAngle) / (gSpotLight.cosFalloffStart - gSpotLight.cosAngle));
        
        float3 spotLight_Diffuse = gMaterial.color.rgb * textureColor.rgb * gSpotLight.color.rgb * spotLight_Cos * gSpotLight.intensity * factor_Spot * falloffFactor;
      
        float3 spotLight_Specular = gSpotLight.color.rgb * gSpotLight.intensity * spotLight_SpecularPow * factor_Spot * float3(1.0f, 1.0f, 1.0f) * falloffFactor;
        
        
       
   
        //if (environment)
       // {
        
        float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
        float3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
        float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);
       
       // }

        

        
        output.color.rgb = directionalLight_Diffuse + directionalLight_Specular + pointLight_Diffuse + pointLight_Specular + spotLight_Diffuse + spotLight_Specular;

        if (environment.useEnvironment)
        {
            output.color.rgb += environmentColor.rgb;
        }
        
        // アルファ（おまけで適用）
        output.color.a = gMaterial.color.a * textureColor.a;

        
       
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
        
    if (output.color.a < 0.5f)
    {
        discard;
    }

    return output;
}
