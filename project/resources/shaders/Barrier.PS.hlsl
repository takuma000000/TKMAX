struct PSInput
{
    float4 pos : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : POSITION0;
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
    float breakProgress;
    float breakEdgeWidth;
    float breakGlowStrength;

    float3 tint;
    float breakNoiseScale;

    float3 breakOrigin;
    float _pad1;
};

float Hash21(float2 p)
{
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 45.32f);
    return frac(p.x * p.y);
}

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

    float lineMask = 1.0f - smoothstep(lineWidth, lineWidth + 0.035f, distToBorder);
    lineMask = pow(saturate(lineMask), 0.65f);

    return lineMask;
}

float CrackNoise(float2 uv, float scale, float width)
{
    float2 p = uv * scale;
    float2 cell = floor(p);
    float2 f = frac(p) - 0.5f;

    float n = Hash21(cell);
    float angle = n * 6.2831853f;
    float2 dir = float2(cos(angle), sin(angle));

    float dist = abs(dot(f, dir));
    float crack = 1.0f - smoothstep(width, width + 0.03f, dist);
    return crack;
}

float4 main(PSInput input) : SV_TARGET
{
    float3 n = normalize(input.normal);
    float3 viewDir = normalize(float3(0.0f, 0.0f, -1.0f));

    float fresnel = 1.0f - saturate(dot(n, -viewDir));
    float rim = pow(fresnel, max(fresnelPower, 0.001f));

    float2 uv = input.texcoord;
    float hexMask = HexLineMask(uv, hexScale, hexLineWidth);

    float3 baseColor = color_.rgb * tint;
    float3 innerColor = baseColor * 0.015f;
    float3 rimColor = baseColor * (rim * 0.65f);

    float3 hexLineColor = lerp(baseColor, float3(1.0f, 0.95f, 1.0f), 0.35f);
    float3 hexColor = hexLineColor * (hexMask * hexGlowStrength * 2.8f);

    float highlightBand = 1.0f - abs(uv.x - 0.30f);
    highlightBand = saturate((highlightBand - 0.86f) * 12.0f);
    float3 highlightColor = float3(1.0f, 0.95f, 1.0f) * highlightBand * 0.55f;

    float breakT = saturate(breakProgress);

// --------------------------------------------------
// ヒビ生成
// --------------------------------------------------
    float crackA = CrackNoise(uv, breakNoiseScale, breakEdgeWidth);
    float crackB = CrackNoise(uv + float2(3.17f, 1.91f), breakNoiseScale * 1.37f, breakEdgeWidth * 0.7f);
    float crackC = HexLineMask(uv, hexScale * 0.8f, hexLineWidth * 1.6f);

    float crackMask = saturate(max(crackA, crackB * 0.75f));
    crackMask = saturate(max(crackMask, crackC * 0.9f));

// ヒビが入るタイミングは前半に寄せる
    float crackPhase = smoothstep(0.00f, 0.28f, breakT);
    float crackVisible = crackMask * crackPhase;

// ヒビの白発光
    float crackGlow = crackVisible * breakGlowStrength * (1.0f - smoothstep(0.22f, 0.55f, breakT));

    // --------------------------------------------------
    // ガラスの破片っぽい「面」単位の割れ
    // ※ フェードではなく、後半で一気に抜く
    // --------------------------------------------------
    float2 shardUV = uv * (breakNoiseScale * 0.42f);
    float2 shardCell = floor(shardUV);
    float shardRnd = Hash21(shardCell);

    // 破片ごとに少し順番をずらす
    float shatterStart = 0.56f + shardRnd * 0.12f;
    float shatterEnd = shatterStart + 0.10f;

    // 0→1で「その破片が飛んだ」判定
    float shardGone = smoothstep(shatterStart, shatterEnd, breakT);

    // ただのフェード感を消すため、しきい値でかなり硬めに抜く
    shardGone = step(0.55f, shardGone);

    // ヒビ周辺は少し先に欠け始める
    float crackChipped = step(0.30f, crackVisible) * smoothstep(0.42f, 0.62f, breakT);

    // 最終的な「消える面」
    float removed = saturate(max(shardGone, crackChipped));

    // --------------------------------------------------
    // パリン瞬間のフラッシュ
    // --------------------------------------------------
    float shatterBurst = smoothstep(0.52f, 0.60f, breakT) * (1.0f - smoothstep(0.60f, 0.72f, breakT));
    float3 shatterFlash = crackVisible.xxx * shatterBurst * 2.0f;

    // --------------------------------------------------
    // 色
    // --------------------------------------------------
    float3 finalColor = innerColor + rimColor + hexColor + highlightColor;
    finalColor += crackGlow.xxx;
    finalColor += shatterFlash;

    // 割れている最中は白く寄せる
    float whitenPhase = smoothstep(0.48f, 0.62f, breakT) * (1.0f - smoothstep(0.78f, 0.92f, breakT));
    float whitenMask = saturate(max(crackVisible, shardGone));
    finalColor = lerp(finalColor, float3(1.0f, 1.0f, 1.0f), whitenPhase * whitenMask);

    finalColor = saturate(finalColor);

    // --------------------------------------------------
    // α
    // 「徐々に薄くする」のをやめる
    // 基本は残して、破片が飛んだところだけ急に消す
    // --------------------------------------------------
    float alphaInner = color_.a * 0.06f;
    float alphaRimVal = color_.a * (rim * 0.35f);
    float alphaHex = hexMask * hexAlpha * 1.6f;
    float alphaHighlight = highlightBand * 0.10f;

    float alpha = saturate(alphaInner + alphaRimVal + alphaHex + alphaHighlight);

    // ヒビ入った後も本体はしばらく残す
    alpha *= (1.0f - removed);

    // 破壊終盤だけ、残骸を少しだけまとめて落とす
    float endKill = smoothstep(0.88f, 1.0f, breakT);
    alpha *= (1.0f - endKill);

    // 完全に飛んだ破片は描かない
    if (alpha <= 0.01f)
    {
        discard;
    }

    return float4(finalColor, saturate(alpha));
}