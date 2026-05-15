#include "Sprite.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
    float4 glowColor;
    float4 glowParam; // x=intensity, y=width, z=threshold, w=softness

    int glowEnabled;
    float3 _pad;

    float4x4 uvTransform;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float2 uv = transformedUV.xy;

    float4 textureColor = gTexture.Sample(gSampler, uv);
    float4 baseColor = gMaterial.color * textureColor;

    float4 finalColor = baseColor;

    float glowIntensity = gMaterial.glowParam.x;
    float glowWidth = gMaterial.glowParam.y;
    float glowThreshold = gMaterial.glowParam.z;
    float glowSoftness = gMaterial.glowParam.w;

    if (gMaterial.glowEnabled != 0 && glowIntensity > 0.0001f)
    {
        uint texW, texH;
        gTexture.GetDimensions(texW, texH);

        float2 texel = 1.0f / float2((float) texW, (float) texH);
        float2 glowStep = texel * max(glowWidth, 0.0f);

        float aC = textureColor.a;
        float aL = gTexture.Sample(gSampler, uv + float2(-glowStep.x, 0.0f)).a;
        float aR = gTexture.Sample(gSampler, uv + float2(glowStep.x, 0.0f)).a;
        float aU = gTexture.Sample(gSampler, uv + float2(0.0f, -glowStep.y)).a;
        float aD = gTexture.Sample(gSampler, uv + float2(0.0f, glowStep.y)).a;
        float aUL = gTexture.Sample(gSampler, uv + float2(-glowStep.x, -glowStep.y)).a;
        float aUR = gTexture.Sample(gSampler, uv + float2(glowStep.x, -glowStep.y)).a;
        float aDL = gTexture.Sample(gSampler, uv + float2(-glowStep.x, glowStep.y)).a;
        float aDR = gTexture.Sample(gSampler, uv + float2(glowStep.x, glowStep.y)).a;

        float aroundMax = max(max(max(aL, aR), max(aU, aD)), max(max(aUL, aUR), max(aDL, aDR)));

        // 輪郭外側にだけ乗るグロー
        float edge = saturate(aroundMax - aC);

        // しきい値とにじみ
        edge = saturate((edge - glowThreshold) * glowSoftness);

        float glowAlpha = edge * glowIntensity * gMaterial.glowColor.a;
        float3 glowRgb = gMaterial.glowColor.rgb * glowAlpha;

        //=========================================================
        // 本体自体も発光させる
        //=========================================================
        // テクスチャ本体のアルファを使って、本体全体を光らせる
        float bodyGlow = aC * glowIntensity;
        // 本体発光
        finalColor.rgb += gMaterial.glowColor.rgb * bodyGlow;
        // 輪郭発光
        finalColor.rgb += glowRgb;

        // アルファは元の輪郭を壊しすぎないよう少しだけ補強
        finalColor.a = saturate(max(finalColor.a, baseColor.a + glowAlpha * 0.35f));
    }

    output.color = finalColor;
    return output;
}