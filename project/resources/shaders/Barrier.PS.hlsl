// =============================================================
// Barrier Pixel Shader
// バリアの描画 + 破壊演出（ヒビ → パリン）
// =============================================================

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : POSITION0; // ワールド座標（破壊中心との距離計算に使用）
};

// =============================================================
// マテリアル
// =============================================================
cbuffer Material : register(b0)
{
    float4 color_; // ベースカラー
    int enableLighting_;
    float3 padding_;
    float4x4 uvTransform_;
    float shininess_;
    float3 padding2_;
};

// =============================================================
// バリア専用パラメータ
// =============================================================
cbuffer BarrierParam : register(b6)
{
    float fresnelPower; // フレネル強さ
    float baseStrength; // 中心強度
    float rimStrength; // 縁の強さ
    float alphaBase;

    float alphaRim;
    float hexScale; // 六角形のスケール
    float hexLineWidth; // 六角形の線幅
    float hexGlowStrength; // 六角形の発光強度

    float hexAlpha;
    float breakProgress; // 破壊進行（0→1）
    float breakEdgeWidth; // ヒビの太さ
    float breakGlowStrength; // ヒビの発光

    float3 tint;
    float breakNoiseScale; // ノイズ密度（ヒビ・破片用）

    float3 breakOrigin; // 破壊の中心
    float _pad1;
    
    float hitFlashTime;
    float3 hitFlashPos;
};

// =============================================================
// ノイズ関数（破片・ヒビ生成用）
// =============================================================
float Hash21(float2 p)
{
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 45.32f);
    return frac(p.x * p.y);
}

// =============================================================
// 六角形距離関数
// =============================================================
float HexDist(float2 p)
{
    p = abs(p);
    return max(dot(p, normalize(float2(1.0f, 1.7320508f))), p.x);
}

// =============================================================
// 六角形の線マスク
// =============================================================
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

// =============================================================
// ヒビノイズ（ランダム方向の割れ線）
// =============================================================
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

// =============================================================
// メイン
// =============================================================
float4 main(PSInput input) : SV_TARGET
{
    float3 n = normalize(input.normal);

    // 視線方向（簡易）
    float3 viewDir = normalize(float3(0.0f, 0.0f, -1.0f));

    // フレネル（縁の発光）
    float fresnel = 1.0f - saturate(dot(n, -viewDir));
    float rim = pow(fresnel, max(fresnelPower, 0.001f));

    float2 uv = input.texcoord;

    // 六角形ライン
    float hexMask = HexLineMask(uv, hexScale, hexLineWidth);

    // ベース色
    float3 baseColor = color_.rgb * tint;

    float3 innerColor = baseColor * 0.015f;
    float3 rimColor = baseColor * (rim * 0.65f);

    // 六角形発光
    float3 hexLineColor = lerp(baseColor, float3(1.0f, 0.95f, 1.0f), 0.35f);
    float3 hexColor = hexLineColor * (hexMask * hexGlowStrength * 2.8f);

    // ハイライト
    float highlightBand = 1.0f - abs(uv.x - 0.30f);
    highlightBand = saturate((highlightBand - 0.86f) * 12.0f);
    float3 highlightColor = float3(1.0f, 0.95f, 1.0f) * highlightBand * 0.55f;

    float breakT = saturate(breakProgress);
    
    // =============================================================
    // ヒットフラッシュ（全体発光）
    // =============================================================
    float hitFlash = 0.0f;
    float hitFlashAlpha = 0.0f;

    if (hitFlashTime >= 0.0f)
    {
    // 最初に強く光って、すぐ減衰
        float t = saturate(hitFlashTime / 0.20f);
        float timeFade = 1.0f - t;
        timeFade = timeFade * timeFade;

    // 全体を白く持ち上げる
        float fullFlash = 1.35f * timeFade;

    // 縁と六角形ラインは少し強め
        float rimBoost = rim * 1.1f * timeFade;
        float hexBoost = hexMask * 0.9f * timeFade;

        hitFlash = fullFlash + rimBoost + hexBoost;
        hitFlashAlpha = 0.30f * timeFade + hexMask * 0.18f * timeFade;
    }
    
    // =============================================================
    // ヒビ生成
    // =============================================================
    float crackA = CrackNoise(uv, breakNoiseScale, breakEdgeWidth);
    float crackB = CrackNoise(uv + float2(3.17f, 1.91f), breakNoiseScale * 1.37f, breakEdgeWidth * 0.7f);
    float crackC = HexLineMask(uv, hexScale * 0.8f, hexLineWidth * 1.6f);

    float crackMask = saturate(max(crackA, crackB * 0.75f));
    crackMask = saturate(max(crackMask, crackC * 0.9f));

    float crackPhase = smoothstep(0.00f, 0.28f, breakT);
    float crackVisible = crackMask * crackPhase;

    float crackGlow = crackVisible * breakGlowStrength * (1.0f - smoothstep(0.22f, 0.55f, breakT));

    // =============================================================
    // 破片分離（パリン）
    // =============================================================
    float2 shardUV = uv * (breakNoiseScale * 0.42f);
    float2 shardCell = floor(shardUV);
    float shardRnd = Hash21(shardCell);

    float shatterStart = 0.56f + shardRnd * 0.12f;
    float shatterEnd = shatterStart + 0.10f;

    float shardGone = smoothstep(shatterStart, shatterEnd, breakT);
    shardGone = step(0.55f, shardGone);

    float crackChipped = step(0.30f, crackVisible) * smoothstep(0.42f, 0.62f, breakT);

    float removed = saturate(max(shardGone, crackChipped));

    // =============================================================
    // パリン瞬間フラッシュ
    // =============================================================
    float shatterBurst = smoothstep(0.52f, 0.60f, breakT) * (1.0f - smoothstep(0.60f, 0.72f, breakT));
    float3 shatterFlash = crackVisible.xxx * shatterBurst * 2.0f;

    // =============================================================
    // 色
    // =============================================================
    float3 finalColor = innerColor + rimColor + hexColor + highlightColor;
    finalColor += crackGlow.xxx;
    finalColor += shatterFlash;

    // キラン追加
    finalColor += float3(1.0f, 1.0f, 1.0f) * hitFlash * 1.2f;

    // 割れ中は白へ寄せる（ガラス感）
    float whitenPhase = smoothstep(0.50f, 0.60f, breakT) * (1.0f - smoothstep(0.72f, 0.84f, breakT));
    float whitenMask = saturate(max(crackVisible, shardGone));
    float whiteAmount = saturate(whitenPhase * whitenMask * 1.35f);

    finalColor = lerp(finalColor, float3(1.0f, 1.0f, 1.0f), whiteAmount);

    finalColor = saturate(finalColor);

    // =============================================================
    // α（透明度）
    // =============================================================
    float alphaInner = color_.a * 0.06f;
    float alphaRimVal = color_.a * (rim * 0.35f);
    float alphaHex = hexMask * hexAlpha * 1.6f;
    float alphaHighlight = highlightBand * 0.10f;

    float alpha = saturate(alphaInner + alphaRimVal + alphaHex + alphaHighlight);

    // 白い瞬間を見やすくする
    alpha = max(alpha, whiteAmount * 0.95f);

    // 破片が飛んだ部分は削除
    alpha *= (1.0f - removed);

    // 最後にまとめて消す
    float endKill = smoothstep(0.90f, 1.0f, breakT);
    alpha *= (1.0f - endKill);
    alpha += hitFlashAlpha;

    if (alpha <= 0.01f)
    {
        discard;
    }

    return float4(finalColor, saturate(alpha));
}