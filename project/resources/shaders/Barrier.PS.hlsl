struct PSInput
{
    float4 pos : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
};

cbuffer Material : register(b0)
{
    float4 color_;
    int enableLighting_;
    float3 padding_;
    float4x4 uvTransform_;
    float shininess_;
    float3 padding2_;
};

cbuffer BarrierParam : register(b6)
{
    float fresnelPower;
    float baseStrength;
    float rimStrength;
    float alphaBase;

    float alphaRim;
    float hexScale;
    float hexLineWidth;
    float hexGlowStrength;

    float hexAlpha;
    float3 _pad0;

    float3 tint;
    float _pad1;
};

float HexDist(float2 p)
{
    p = abs(p);
    return max(dot(p, normalize(float2(1.0f, 1.7320508f))), p.x);
}

float HexLineMask(float2 uv, float scale, float lineWidth)
{
    uv *= scale;

    float2 gvA = frac(float2(uv.x, uv.y / 0.8660254f)) - 0.5f;
    float2 gvB = frac(float2(uv.x + 0.5f, uv.y / 0.8660254f + 0.5f)) - 0.5f;

    float2 gv = dot(gvA, gvA) < dot(gvB, gvB) ? gvA : gvB;
    gv.y *= 0.8660254f;

    float d = HexDist(gv);
    float distToBorder = abs(0.5f - d);

    // まずは絶対見えるように太め
    float lineMask = 1.0f - smoothstep(lineWidth, lineWidth + 0.035f, distToBorder);

    // 線をさらに強調
    lineMask = pow(saturate(lineMask), 0.65f);

    return lineMask;
}

float4 main(PSInput input) : SV_TARGET
{
    float3 n = normalize(input.normal);
    float3 viewDir = normalize(float3(0.0f, 0.0f, -1.0f));

    float fresnel = 1.0f - saturate(dot(n, -viewDir));
    float rim = pow(fresnel, max(fresnelPower, 0.001f));

    float2 uv = input.texcoord;

    // 六角形ラインを全面に出す
    float hexMask = HexLineMask(uv, hexScale, hexLineWidth);

    float3 baseColor = color_.rgb * tint;

    // 中心の面はかなり薄く
    float3 innerColor = baseColor * 0.015f;

    // 外周
    float3 rimColor = baseColor * (rim * 0.65f);

    // 六角形ラインを主役にする
    float3 hexLineColor = lerp(baseColor, float3(1.0f, 0.95f, 1.0f), 0.35f);
    float3 hexColor = hexLineColor * (hexMask * hexGlowStrength * 2.8f);

    // テカリを強めに1本
    float highlightBand = 1.0f - abs(uv.x - 0.30f);
    highlightBand = saturate((highlightBand - 0.86f) * 12.0f);
    float3 highlightColor = float3(1.0f, 0.95f, 1.0f) * highlightBand * 0.55f;

    float3 finalColor = innerColor + rimColor + hexColor + highlightColor;
    finalColor = saturate(finalColor);

    // アルファも六角形優先
    float alphaInner = color_.a * 0.06f;
    float alphaRimVal = color_.a * (rim * 0.35f);
    float alphaHex = hexMask * hexAlpha * 1.6f;
    float alphaHighlight = highlightBand * 0.10f;

    float alpha = saturate(alphaInner + alphaRimVal + alphaHex + alphaHighlight);

    return float4(finalColor, alpha);
}