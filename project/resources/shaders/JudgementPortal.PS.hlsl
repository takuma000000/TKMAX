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

	// 渦巻き歪みっぽくUVを回す
    float swirlPower = lerp(1.8f, 4.8f, charge);
    float swirl = a + (1.0f - d) * swirlPower + gTime * lerp(0.8f, 2.4f, charge);

    float2 swirlUV = float2(
		cos(swirl),
		sin(swirl)
	) * d * 0.5f + 0.5f;

    float n0 = fbm(swirlUV * 7.0f + float2(gTime * 0.12f, -gTime * 0.08f));
    float n1 = fbm(uv * 18.0f + float2(-gTime * 0.25f, gTime * 0.18f));

	// 中心の黒い穴
    float blackCore = 1.0f - smoothstep(0.0f, lerp(0.28f, 0.42f, charge), d);

	// 外側の歪みリング
    float outer = ring(d, lerp(0.58f, 0.48f, charge), 0.035f + n0 * 0.018f);
    float inner = ring(d, lerp(0.32f, 0.24f, charge), 0.025f + n1 * 0.012f);

	// 吸い込み線
    float spokes = sin(a * 14.0f - gTime * lerp(2.0f, 7.0f, charge) + n0 * 4.0f);
    spokes = smoothstep(0.72f, 1.0f, spokes);
    spokes *= 1.0f - smoothstep(0.18f, 0.72f, d);

	// 外周のぐにゃぐにゃ境界
    float edgeNoise = fbm(float2(a * 2.0f, d * 8.0f + gTime * 0.2f));
    float edge = 1.0f - smoothstep(0.72f + edgeNoise * 0.08f, 0.88f, d);

	// 発射直前の脈動
    float pulse = pow(saturate(sin(gTime * lerp(4.0f, 16.0f, charge)) * 0.5f + 0.5f), 5.0f);
    float chargeGlow = lerp(0.35f, 1.4f, charge) + pulse * charge;

    float energy = outer * 1.1f + inner * 0.75f + spokes * 0.55f + n1 * 0.12f;
    energy *= edge;

    float3 deep = float3(0.0f, 0.004f, 0.018f);
    float3 blue = float3(0.02f, 0.35f, 1.25f);
    float3 cyan = float3(0.10f, 1.0f, 0.85f);
    float3 purple = float3(0.35f, 0.04f, 0.85f);

    float3 color = deep;
    color += blue * outer * 1.2f;
    color += cyan * inner * 0.8f;
    color += purple * spokes * 0.7f;
    color += float3(0.04f, 0.18f, 0.50f) * n0 * 0.25f;

	// 中心は黒く沈ませる
    color = lerp(color, float3(0.0f, 0.0f, 0.0f), blackCore * 0.85f);

	// 周囲だけ光る
    color *= chargeGlow;

    float alpha = saturate(energy * 0.8f + outer * 0.35f + inner * 0.25f);
    alpha *= edge;
    alpha *= gIntensity;

	// 中心の黒穴は透明ではなく、少しだけ残す
    alpha = max(alpha, blackCore * 0.22f * gIntensity);

    return float4(color, alpha);
}