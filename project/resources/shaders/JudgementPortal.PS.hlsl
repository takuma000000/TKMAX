struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

cbuffer JudgementPortalCB : register(b0)
{
    float4x4 gViewProj;

    float3 gCenterWS;
    float gTime;

    float3 gCamRight;
    float gSize;

    float3 gCamUp;
    float gCharge01;

    float3 gCamFwd;
    float gIntensity;
};

float hash21(float2 p)
{
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 45.32f);
    return frac(p.x * p.y);
}

float noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    float a = hash21(i);
    float b = hash21(i + float2(1.0f, 0.0f));
    float c = hash21(i + float2(0.0f, 1.0f));
    float d = hash21(i + float2(1.0f, 1.0f));

    float2 u = f * f * (3.0f - 2.0f * f);

    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

float fbm(float2 p)
{
    float v = 0.0f;
    float a = 0.5f;

	[unroll]
    for (int i = 0; i < 5; ++i)
    {
        v += noise(p) * a;
        p *= 2.03f;
        a *= 0.5f;
    }

    return v;
}

float ring(float d, float radius, float width)
{
    return 1.0f - smoothstep(width, width + 0.018f, abs(d - radius));
}

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.uv;
    float2 p = uv * 2.0f - 1.0f;

    float d = length(p);
    float a = atan2(p.y, p.x);

    float charge = saturate(gCharge01);

    // 円形マスク。まず「一個の塊」として見せる
    float body = 1.0f - smoothstep(0.62f, 0.82f, d);

    // 中心の穴。かなり大きめ
    float core = 1.0f - smoothstep(0.0f, lerp(0.38f, 0.50f, charge), d);

    // 外側の厚い発光。リングではなく、塊の輪郭
    float rim = smoothstep(0.30f, 0.72f, d) * (1.0f - smoothstep(0.72f, 0.90f, d));

    // 渦ノイズ
    float swirlPower = lerp(2.8f, 6.0f, charge);
    float swirl = a + (1.0f - d) * swirlPower + gTime * lerp(1.0f, 3.0f, charge);

    float2 swirlUV = float2(cos(swirl), sin(swirl)) * d * 0.5f + 0.5f;

    float n0 = fbm(swirlUV * 8.0f + float2(gTime * 0.15f, -gTime * 0.10f));
    float n1 = fbm(uv * 20.0f + float2(-gTime * 0.25f, gTime * 0.18f));

    // 塊内部のうねり
    float innerStorm = smoothstep(0.25f, 0.95f, n0) * body;

    // 中心へ吸い込まれる筋
    float spokes = sin(a * 18.0f - gTime * lerp(3.0f, 9.0f, charge) + n0 * 5.0f);
    spokes = smoothstep(0.68f, 1.0f, spokes);
    spokes *= 1.0f - smoothstep(0.10f, 0.75f, d);

    // 境界をグニャらせる
    float edgeNoise = fbm(float2(a * 3.0f, d * 9.0f + gTime * 0.25f));
    float edge = 1.0f - smoothstep(0.72f + edgeNoise * 0.08f, 0.92f, d);

    // 発射前の圧
    float pulse = pow(saturate(sin(gTime * lerp(5.0f, 18.0f, charge)) * 0.5f + 0.5f), 5.0f);
    float chargeGlow = lerp(0.85f, 2.2f, charge) + pulse * charge * 1.2f;

    // 色：背景に負けない黒赤白
    float3 black = float3(0.0f, 0.0f, 0.0f);
    float3 darkRed = float3(0.20f, 0.0f, 0.015f);
    float3 red = float3(2.2f, 0.04f, 0.03f);
    float3 hot = float3(3.0f, 1.8f, 1.1f);
    float3 purple = float3(0.55f, 0.02f, 0.85f);

    float3 color = black;

    // まず全体を黒赤の塊にする
    color += darkRed * body * 1.4f;

    // 外周を太く発光
    color += red * rim * 1.8f;

    // 内部の渦
    color += purple * innerStorm * 0.75f;

    // 吸い込み線
    color += hot * spokes * 0.7f;

    // 中心は強制的に黒く沈める
    color = lerp(color, black, core * 0.95f);

    // でも中心の周りに白赤い圧縮光
    float coreRim = smoothstep(0.34f, 0.52f, d) * (1.0f - smoothstep(0.52f, 0.68f, d));
    color += hot * coreRim * lerp(0.35f, 1.0f, charge);

    color *= chargeGlow;

    // alphaも塊として強めに出す
    float alpha = saturate(
        body * 0.72f +
        rim * 0.55f +
        innerStorm * 0.25f +
        spokes * 0.25f +
        core * 0.55f
    );

    alpha *= edge;
    alpha *= gIntensity;

    return float4(color, alpha);
}