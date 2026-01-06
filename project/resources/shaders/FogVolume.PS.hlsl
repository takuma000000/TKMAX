struct PSIn
{
    float4 svpos : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float sliceT : TEXCOORD2;
};

cbuffer FogVolumeCB : register(b0)
{
    float4x4 ViewProj;

    float3 CenterWS;
    float _pad0;
    float3 HalfSizeWS;
    float Density;

    float3 CamRightWS;
    float _pad1;
    float3 CamUpWS;
    float _pad2;
    float3 CamFwdWS;
    float _pad3;

    uint SliceCount;
    float Time;
    float NoiseScale;
    float NoiseSpeed;

    float3 FogColor;
    float Softness;

    // 0..1（体積の下端〜上端）
    float FogStart;
    float FogEnd;

    float NoiseStrength; // ムラ強さ
    float WorldScale; // ノイズ座標の基準スケール

    float3 WorldPos; // ノイズ基準点（基本 CenterWS と同期推奨）
    float _padX;
};

// ----------------------------
// 2D noise
// ----------------------------
float hash21(float2 p)
{
    p = frac(p * 0.3183099 + float2(0.71, 0.113));
    p *= 17.0;
    return frac(p.x * p.y * (p.x + p.y));
}

float smoothNoise2D(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    float a = hash21(i);
    float b = hash21(i + float2(1.0, 0.0));
    float c = hash21(i + float2(0.0, 1.0));
    float d = hash21(i + float2(1.0, 1.0));

    float2 u = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

float fbm2(float2 p)
{
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; i++)
    {
        v += a * smoothNoise2D(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v; // だいたい 0..1
}

// 箱の端フェード（体積感）
float boxFade(float3 p, float3 center, float3 halfSize, float softness)
{
    float3 q = abs((p - center) / max(halfSize, 1e-3));
    float d = max(q.x, max(q.y, q.z)); // 0=中心, 1=箱の面
    float a = saturate(1.0 - d);
    return pow(a, max(softness, 0.001));
}

float4 main(PSIn input) : SV_TARGET
{
    // ============================================================
    // もくもく煙フォグ（縦スジ防止 + 濃さ安定）
    // ============================================================

    // 端フェード（体積の核）
    float edge = boxFade(input.worldPos, CenterWS, HalfSizeWS, Softness);

    // 高さ 0..1（0=下端, 1=上端）
    float y0 = CenterWS.y - HalfSizeWS.y;
    float y1 = CenterWS.y + HalfSizeWS.y;
    float h01 = (y1 > y0 + 1e-5f) ? saturate((input.worldPos.y - y0) / (y1 - y0)) : 0.5f;

    // 下が濃い→上で消える
    float heightFade = 1.0f;
    if (FogEnd > FogStart + 1e-5f)
    {
        heightFade = 1.0f - smoothstep(FogStart, FogEnd, h01);
    }

    // ノイズ座標（体積と一緒に動かす）
    float ws = max(WorldScale, 1e-3f);
    float3 local = (input.worldPos - WorldPos) / ws;

    // スライスごとに少しズラす（板感を弱める）
    float sliceOffset = (input.sliceT - 0.5f) * 0.35f;
    local += CamFwdWS * sliceOffset;

    // ベース濃度（体積・高さ・端フェード）
    float sigma = Density;
    sigma *= edge;
    sigma *= heightFade;

    // ------------------------------------------------------------
    // もくもく煙ノイズ：
    //   xzだけにしない（xy/yzも混ぜる）→縦スジを潰す
    //   domain warp で流れ感
    // ------------------------------------------------------------
    float t = Time * NoiseSpeed;
    float ns = saturate(NoiseStrength);

    // 3D座標（NoiseScaleは“塊の大きさ”）
    float3 p3 = local * NoiseScale;

    // ゆっくり流す（煙は遅め）
    p3 += float3(t, t * 0.15f, -t * 0.7f);

    // domain warp（3面平均）
    float2 w_xz = float2(fbm2(p3.xz * 0.35f + 13.7f), fbm2(p3.xz * 0.35f + 57.2f));
    float2 w_xy = float2(fbm2(p3.xy * 0.35f + 31.4f), fbm2(p3.xy * 0.35f + 92.8f));
    float2 w_yz = float2(fbm2(p3.yz * 0.35f + 11.1f), fbm2(p3.yz * 0.35f + 44.4f));

    float2 warp2 = (w_xz + w_xy + w_yz) / 3.0f;
    warp2 = (warp2 - 0.5f) * 2.0f; // -1..1

    // warp量（強すぎると破綻するので控えめ）
    float2 pxz = p3.xz + warp2 * (0.55f * ns);
    float2 pxy = p3.xy + warp2 * (0.55f * ns);
    float2 pyz = p3.yz + warp2 * (0.55f * ns);

    // 低周波メイン（塊）を3面で平均
    float low = (fbm2(pxz * 0.22f) + fbm2(pxy * 0.22f + 19.0f) + fbm2(pyz * 0.22f + 37.0f)) / 3.0f;
    float mid = (fbm2(pxz * 0.60f + 21.3f) + fbm2(pxy * 0.60f + 41.7f) + fbm2(pyz * 0.60f + 88.2f)) / 3.0f;
    float high = (fbm2(pxz * 1.30f + 90.1f) + fbm2(pxy * 1.30f + 12.4f) + fbm2(pyz * 1.30f + 55.5f)) / 3.0f;

    // 低周波主役で塊感
    float field = low;
    field = lerp(field, field * 0.78f + mid * 0.22f, 0.85f);
    field = lerp(field, field * 0.92f + high * 0.08f, 0.60f);

    // コントラスト（ムラを立てる）
    float contrast = lerp(1.10f, 2.10f, ns);
    field = pow(saturate(field), contrast);

    // 塊の輪郭（ここ狭めるほどモコモコ）
    field = smoothstep(0.22f, 0.82f, field);

    // ムラで濃さ（煙は強めでOK）
    sigma *= lerp(0.45f, 2.10f, field);

    // ------------------------------------------------------------
    // 濃さ（薄い問題の本丸）
    // step = 体積の奥行き / スライス数
    // ------------------------------------------------------------
    // abs() は HLSL で float3 にも効く
    float halfD = dot(abs(CamFwdWS), HalfSizeWS); // カメラ前後方向の半径
    float pathLen = max(halfD * 2.0f, 1e-3f); // 体積を貫く距離
    float step = pathLen / max((float) SliceCount, 1.0f);

    // 表示用補正（薄いなら 0.035→0.05 に上げる）
    step *= 0.035f;

    float alpha = 1.0f - exp(-sigma * step);
    alpha = saturate(alpha);

    return float4(FogColor, alpha);
}
