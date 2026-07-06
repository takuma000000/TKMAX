struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

cbuffer JudgementBackgroundCB : register(b0)
{
    float4x4 gViewProj;

    float3 gCenterWS;
    float gTime;

    float3 gCamRight;
    float gIntensity;

    float3 gCamUp;
    float gWidth;

    float3 gCamFwd;
    float gHeight;
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

float vortex(float2 uv, float2 center, float radius, float speed, float density)
{
    float2 p = uv - center;
    float d = length(p);
    float a = atan2(p.y, p.x);

    float swirl = sin(a * density + d * 42.0f - gTime * speed);
    float ring = 1.0f - smoothstep(radius * 0.55f, radius, d);
    float hole = smoothstep(0.035f, radius * 0.25f, d);

    float n = fbm(uv * 8.0f + float2(gTime * 0.06f, -gTime * 0.04f));

    return saturate(swirl * 0.5f + 0.5f) * ring * hole * (0.55f + n * 0.65f);
}

float crack(float2 uv, float x, float yOffset, float height, float seed)
{
    float y = uv.y + sin(uv.y * 18.0f + seed) * 0.018f;
    float lineX = x + sin(y * 25.0f + seed + gTime * 0.7f) * 0.035f;

    float w = 0.008f + noise(float2(y * 18.0f, seed)) * 0.018f;

    float crackLine = 1.0f - smoothstep(0.0f, w, abs(uv.x - lineX));

    float range =
        smoothstep(yOffset - height, yOffset, uv.y) *
        (1.0f - smoothstep(yOffset, yOffset + height, uv.y));

    float flicker = 0.45f + 0.55f * sin(gTime * 8.0f + seed * 12.0f);

    float core = 1.0f - smoothstep(0.0f, w * 0.35f, abs(uv.x - lineX));

    return (crackLine * 0.65f + core * 1.5f) * range * flicker;
}

float ringFx(float2 uv, float2 center, float radius, float width, float rotateSpeed, float seed)
{
    float2 p = uv - center;
    float d = length(p);
    float angle = atan2(p.y, p.x);

    angle += gTime * rotateSpeed;

    float ring = 1.0f - smoothstep(width, width + 0.012f, abs(d - radius));

    float breakMask =
        0.5f +
        0.5f * sin(angle * 13.0f + gTime * 2.0f + seed);

    float noiseMask = fbm(float2(angle * 1.5f, d * 12.0f + seed));

    float broken = smoothstep(0.22f, 0.75f, breakMask * 0.65f + noiseMask * 0.55f);

    return ring * broken;
}

float edgeInvasion(float2 uv)
{
    float2 p = uv * 2.0f - 1.0f;

    float edge =
        smoothstep(0.35f, 1.05f, abs(p.x)) +
        smoothstep(0.45f, 1.05f, abs(p.y));

    float n = fbm(uv * 9.0f + float2(gTime * 0.08f, -gTime * 0.05f));

    return saturate(edge * n);
}

float tentacleFlow(float2 uv)
{
    float2 p = uv - 0.5f;

    float a = atan2(p.y, p.x);
    float d = length(p);

    float wave =
        sin(a * 5.0f + d * 22.0f - gTime * 1.1f) *
        sin(uv.y * 18.0f + gTime * 0.8f);

    float mask = smoothstep(0.15f, 0.9f, d) * (1.0f - smoothstep(0.65f, 1.0f, d));

    return smoothstep(0.45f, 0.9f, wave) * mask;
}

float blackHole(float2 uv, float2 center, float radius, float seed)
{
    float2 p = uv - center;
    float d = length(p);
    float a = atan2(p.y, p.x);

    float core = 1.0f - smoothstep(0.0f, radius * 0.45f, d);
    float edge = 1.0f - smoothstep(radius * 0.35f, radius, d);

    float swirl = sin(a * 10.0f + d * 55.0f - gTime * 1.2f + seed);
    swirl = swirl * 0.5f + 0.5f;

    return saturate(core * 1.4f + edge * swirl * 0.45f);
}

float darkVein(float2 uv, float seed)
{
    float y = uv.y + sin(uv.x * 10.0f + seed) * 0.08f;
    float n = fbm(float2(uv.x * 8.0f + seed, uv.y * 18.0f - gTime * 0.25f));

    float vein = sin(y * 32.0f + n * 5.0f + seed);
    vein = 1.0f - smoothstep(0.0f, 0.10f, abs(vein));

    float broken = smoothstep(0.35f, 0.85f, n);

    return vein * broken;
}

float abyssEye(float2 uv, float2 center, float scale, float seed)
{
    float2 p = uv - center;
    p.x /= scale;
    p.y /= scale * 0.42f;

    float d = length(p);

    float eyeShape = 1.0f - smoothstep(0.85f, 1.0f, d);
    float innerCut = smoothstep(0.18f, 0.45f, d);

    float pupil = 1.0f - smoothstep(0.05f, 0.22f, length(p));

    float blink = pow(saturate(sin(gTime * 0.55f + seed) * 0.5f + 0.5f), 18.0f);

    return eyeShape * innerCut * blink * 0.55f + pupil * blink * 0.8f;
}

float giantOuterRing(float2 uv, float2 center, float radius, float width, float seed)
{
    float2 p = uv - center;
    float d = length(p);
    float a = atan2(p.y, p.x);

    float ring = 1.0f - smoothstep(width, width + 0.018f, abs(d - radius));

    float broken = 0.5f + 0.5f * sin(a * 18.0f + gTime * 0.6f + seed);
    float n = fbm(float2(a * 2.0f + seed, d * 18.0f - gTime * 0.15f));

    return ring * smoothstep(0.18f, 0.78f, broken * 0.55f + n * 0.65f);
}

float radialCrack(float2 uv, float2 origin, float angle, float lengthValue, float seed)
{
    float2 p = uv - origin;

    float2 dir = float2(cos(angle), sin(angle));
    float2 side = float2(-dir.y, dir.x);

    float along = dot(p, dir);
    float widthValue = abs(dot(p, side));

    float jag = sin(along * 55.0f + seed + gTime * 0.8f) * 0.012f;
    widthValue = abs(widthValue + jag);

    float crackLine = 1.0f - smoothstep(0.0f, 0.012f, widthValue);

    float range =
        smoothstep(0.0f, 0.08f, along) *
        (1.0f - smoothstep(lengthValue * 0.82f, lengthValue, along));

    float n = fbm(float2(along * 20.0f, seed));

    return crackLine * range * (0.5f + n);
}

float debrisField(float2 uv, float seed)
{
    float2 grid = uv * 32.0f;
    float2 id = floor(grid);
    float2 f = frac(grid) - 0.5f;

    float h = hash21(id + seed);
    float appear = step(0.86f, h);

    float2 drift = float2(
        sin(gTime * 0.18f + h * 12.0f),
        cos(gTime * 0.12f + h * 8.0f)
    ) * 0.18f;

    float d = length(f + drift);

    float dotShape = 1.0f - smoothstep(0.02f, 0.10f, d);

    return dotShape * appear;
}

float riftMist(float2 uv)
{
    float n0 = fbm(uv * 3.0f + float2(gTime * 0.025f, -gTime * 0.02f));
    float n1 = fbm(uv * 7.0f + float2(-gTime * 0.04f, gTime * 0.03f));
    float n2 = fbm(uv * 15.0f + float2(gTime * 0.07f, gTime * 0.02f));

    return n0 * 0.45f + n1 * 0.35f + n2 * 0.20f;
}

float massiveArc(float2 uv, float2 center, float radius, float widthValue, float seed)
{
    float2 p = uv - center;
    float d = length(p);
    float a = atan2(p.y, p.x);

    float arc = 1.0f - smoothstep(widthValue, widthValue + 0.02f, abs(d - radius));
    float gate = smoothstep(-0.85f, -0.15f, sin(a * 2.0f + seed));
    float broken = smoothstep(0.30f, 0.85f, fbm(float2(a * 1.8f + seed, d * 10.0f + gTime * 0.08f)));

    return arc * gate * broken;
}

float distantStarSink(float2 uv)
{
    float2 grid = uv * 70.0f;
    float2 id = floor(grid);
    float2 f = frac(grid) - 0.5f;

    float h = hash21(id);
    float appear = step(0.955f, h);

    float twinkle = 0.45f + 0.55f * sin(gTime * (0.6f + h * 2.0f) + h * 20.0f);
    float d = length(f);

    return (1.0f - smoothstep(0.01f, 0.055f, d)) * appear * twinkle;
}

float4 main(PSInput input) : SV_TARGET
{
    // 背景全体の呼吸
    float pulse = 1.0f + sin(gTime * 0.8f) * 0.025f;

    float2 uv = (input.uv - 0.5f) * pulse + 0.5f;
    float2 p = uv * 2.0f - 1.0f;

    float vignette = 1.0f - smoothstep(0.45f, 1.35f, length(p));

    float n1 = fbm(uv * 4.0f + float2(gTime * 0.04f, -gTime * 0.025f));
    float n2 = fbm(uv * 12.0f + float2(-gTime * 0.08f, gTime * 0.04f));
    float n3 = fbm(uv * 28.0f + float2(gTime * 0.14f, gTime * 0.10f));

    // 巨大渦
    float v0 = vortex(uv, float2(0.50f, 0.48f), 0.62f, 1.00f, 8.0f);
    float v1 = vortex(uv, float2(0.20f, 0.30f), 0.36f, -1.35f, 9.0f);
    float v2 = vortex(uv, float2(0.80f, 0.68f), 0.38f, 1.55f, 10.0f);
    float v3 = vortex(uv, float2(0.18f, 0.78f), 0.28f, 1.20f, 11.0f);
    float v4 = vortex(uv, float2(0.86f, 0.25f), 0.30f, -1.10f, 12.0f);

    float energy = v0 * 1.10f + v1 * 0.60f + v2 * 0.65f + v3 * 0.45f + v4 * 0.45f;

    // 空間亀裂
    float c0 = crack(uv, 0.25f, 0.52f, 0.46f, 1.0f);
    float c1 = crack(uv, 0.70f, 0.46f, 0.42f, 3.0f);
    float c2 = crack(uv, 0.52f, 0.64f, 0.30f, 7.0f);
    float c3 = crack(uv, 0.13f, 0.40f, 0.34f, 11.0f);
    float c4 = crack(uv, 0.88f, 0.58f, 0.36f, 17.0f);

    float cracks = c0 + c1 + c2 + c3 * 0.8f + c4 * 0.8f;

    // 巨大リング
    float r0 = ringFx(uv, float2(0.50f, 0.50f), 0.34f, 0.014f, 0.25f, 1.0f);
    float r1 = ringFx(uv, float2(0.24f, 0.35f), 0.22f, 0.010f, -0.35f, 2.0f);
    float r2 = ringFx(uv, float2(0.78f, 0.66f), 0.24f, 0.012f, 0.32f, 3.0f);
    float r3 = ringFx(uv, float2(0.84f, 0.28f), 0.18f, 0.009f, -0.42f, 4.0f);

    float rings = r0 * 0.85f + r1 * 0.55f + r2 * 0.60f + r3 * 0.45f;

    // 画面外まで続く巨大リング
    float outerRing =
        giantOuterRing(uv, float2(0.50f, 0.52f), 0.62f, 0.018f, 1.0f) +
        giantOuterRing(uv, float2(0.18f, 0.72f), 0.42f, 0.014f, 3.0f) * 0.65f +
        giantOuterRing(uv, float2(0.88f, 0.25f), 0.46f, 0.014f, 5.0f) * 0.65f;

    // 画面外から中心へ向かう巨大亀裂
    float bigCracks =
        radialCrack(uv, float2(-0.08f, 0.18f), 0.35f, 0.95f, 1.0f) +
        radialCrack(uv, float2(1.08f, 0.72f), 3.55f, 0.95f, 4.0f) +
        radialCrack(uv, float2(0.20f, -0.10f), 1.05f, 0.85f, 8.0f) * 0.8f +
        radialCrack(uv, float2(0.85f, 1.10f), -2.05f, 0.85f, 12.0f) * 0.8f;

    // さらに大きな画面外アーク
    float hugeArcs =
        massiveArc(uv, float2(0.52f, 0.52f), 0.78f, 0.020f, 1.0f) * 0.75f +
        massiveArc(uv, float2(0.18f, 0.70f), 0.62f, 0.016f, 4.0f) * 0.45f +
        massiveArc(uv, float2(0.92f, 0.25f), 0.58f, 0.016f, 7.0f) * 0.45f;

    // 流れるエネルギー
    float flow = sin((uv.x + uv.y * 0.35f) * 36.0f - gTime * 2.4f);
    flow = smoothstep(0.76f, 1.0f, flow);
    flow *= 0.5f + n2 * 0.8f;

    // 端から侵食
    float invasion = edgeInvasion(uv);

    // 触手っぽい流れ
    float tentacle = tentacleFlow(uv);

    // 黒い穴・闇の血管・深淵の目
    float hole0 = blackHole(uv, float2(0.50f, 0.49f), 0.20f, 1.0f);
    float hole1 = blackHole(uv, float2(0.20f, 0.30f), 0.13f, 2.0f);
    float hole2 = blackHole(uv, float2(0.80f, 0.68f), 0.14f, 3.0f);
    float holes = hole0 * 1.0f + hole1 * 0.65f + hole2 * 0.65f;

    float veins =
        darkVein(uv, 1.0f) * 0.45f +
        darkVein(uv + float2(0.13f, 0.07f), 4.0f) * 0.35f +
        darkVein(uv + float2(-0.18f, 0.11f), 8.0f) * 0.30f;

    float eye =
        abyssEye(uv, float2(0.50f, 0.46f), 0.22f, 2.0f) +
        abyssEye(uv, float2(0.74f, 0.32f), 0.13f, 6.0f) * 0.45f;

    // 浮遊する破片
    float debris =
        debrisField(uv + float2(gTime * 0.006f, -gTime * 0.004f), 1.0f) * 0.7f +
        debrisField(uv * 1.35f + float2(-gTime * 0.004f, gTime * 0.006f), 9.0f) * 0.45f;

    // 多層の黒い霧
    float mist = riftMist(uv);

    // 遠くで沈む星/異物
    float starSink = distantStarSink(uv + float2(gTime * 0.002f, -gTime * 0.002f));

    float glow =
        energy * 0.85f +
        cracks * 1.45f +
        bigCracks * 1.25f +
        rings * 0.85f +
        outerRing * 0.65f +
        hugeArcs * 0.55f +
        flow * 0.30f +
        tentacle * 0.35f +
        debris * 0.25f +
        starSink * 0.18f +
        eye * 0.45f +
        n3 * 0.08f;

    // 深海ベース
    float depthGrad = smoothstep(1.0f, -0.25f, uv.y);
    float dark = 0.18f + n1 * 0.22f;

    float3 baseColor = float3(0.004f, 0.012f, 0.045f) * (dark + depthGrad * 0.35f);

    float3 blue = float3(0.02f, 0.22f, 1.10f) * energy;
    float3 green = float3(0.04f, 1.20f, 0.72f) * cracks;
    float3 cyan = float3(0.16f, 0.85f, 1.30f) * rings;
    float3 purple = float3(0.45f, 0.05f, 0.95f) * (flow + tentacle * 0.7f);

    float3 color = baseColor + blue + green + cyan + purple;

    // 画面外まで続く巨大リング
    color += float3(0.05f, 0.65f, 1.25f) * outerRing * 0.75f;

    // さらに外側の巨大アーク
    color += float3(0.04f, 0.35f, 1.05f) * hugeArcs * 0.55f;

    // 外側から走る巨大亀裂
    color += float3(0.02f, 1.10f, 0.82f) * bigCracks * 1.15f;

    // 漂う空間破片
    color += float3(0.18f, 0.42f, 0.95f) * debris * 0.45f;

    // 遠景の微光
    color += float3(0.08f, 0.18f, 0.35f) * starSink * 0.25f;

    // 多層の黒い霧で全体を重くする
    color = lerp(color, float3(0.0f, 0.004f, 0.018f), mist * 0.22f);

    // 黒い穴が発光を吸う
    color = lerp(color, float3(0.0f, 0.0f, 0.004f), saturate(holes) * 0.75f);

    // 黒紫の血管
    color += float3(0.06f, 0.0f, 0.10f) * veins * 0.7f;

    // たまに見える深淵の目
    color += float3(0.02f, 0.22f, 0.55f) * eye;
    color = lerp(color, float3(0.0f, 0.0f, 0.0f), eye * 0.18f);

    // 端は黒く侵食
    color = lerp(color, float3(0.0f, 0.0f, 0.012f), invasion * 0.45f);

    // 闇は沈めて、発光だけ残す
    float glowBoost = saturate(glow);
    color *= lerp(0.42f, 1.45f, glowBoost);

    // たまに脈動で光る
    float heartbeat = pow(saturate(sin(gTime * 2.1f) * 0.5f + 0.5f), 8.0f);
    color += float3(0.04f, 0.10f, 0.25f) * heartbeat * energy;

    float alpha = saturate((glow * 0.58f + rings * 0.18f + outerRing * 0.20f + hugeArcs * 0.16f + 0.10f) * vignette);
    alpha += invasion * 0.08f;
    alpha += debris * 0.04f;
    alpha += starSink * 0.03f;
    alpha *= gIntensity;

    return float4(color, alpha);
}
