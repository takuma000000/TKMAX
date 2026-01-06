struct PSIn
{
    float4 svpos : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float sliceT : TEXCOORD2;
};

cbuffer SmokeVolumeCB : register(b0)
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
    float BaseScale;
    float FlowSpeed;

    float DetailScale;
    float DetailStrength;
    float Threshold;
    float Softness;

    float3 SmokeColor;
    float AlphaMax;

    float RiseSpeed;
    float3 _padX;
};

// ---- 3D value noise ----
float hash31(float3 p)
{
    p = frac(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return frac((p.x + p.y) * p.z);
}

float noise3(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    float n000 = hash31(i + float3(0, 0, 0));
    float n100 = hash31(i + float3(1, 0, 0));
    float n010 = hash31(i + float3(0, 1, 0));
    float n110 = hash31(i + float3(1, 1, 0));
    float n001 = hash31(i + float3(0, 0, 1));
    float n101 = hash31(i + float3(1, 0, 1));
    float n011 = hash31(i + float3(0, 1, 1));
    float n111 = hash31(i + float3(1, 1, 1));

    float n00 = lerp(n000, n100, f.x);
    float n10 = lerp(n010, n110, f.x);
    float n01 = lerp(n001, n101, f.x);
    float n11 = lerp(n011, n111, f.x);

    float n0 = lerp(n00, n10, f.y);
    float n1 = lerp(n01, n11, f.y);

    return lerp(n0, n1, f.z);
}

float fbm(float3 p)
{
    float a = 0.5;
    float s = 0.0;
    for (int i = 0; i < 5; i++)
    {
        s += a * noise3(p);
        p *= 2.02;
        a *= 0.5;
    }
    return s;
}

float4 main(PSIn i) : SV_TARGET
{
    // ===== ノイズ座標 =====
    float3 p = i.worldPos;

    // 画面手前へ流す（-CamFwd） + ちょい上昇
    float3 flow = (-CamFwdWS) * FlowSpeed + float3(0, 1, 0) * RiseSpeed;
    p += flow * Time;

    // モクモク：大きい塊 + 細部
    float baseN = fbm(p * BaseScale);
    float detailN = fbm(p * DetailScale);

    // 雲化（閾値 + ぼかし）
    float cloud = smoothstep(Threshold - Softness, Threshold + Softness, baseN);

    // ディテール混ぜ
    float detail = smoothstep(0.35, 0.85, detailN);
    cloud *= lerp(1.0, detail, saturate(DetailStrength));

    // スライス端の寄与を少し落とす（薄くなる）
    float sliceFade = smoothstep(0.0, 0.12, i.sliceT) * (1.0 - smoothstep(0.88, 1.0, i.sliceT));
    cloud *= sliceFade;

    float a = saturate(cloud * Density) * AlphaMax;

    return float4(SmokeColor, a);
}