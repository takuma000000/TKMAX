struct VSIn
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VSOut
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

VSOut main(VSIn input, uint instanceId : SV_InstanceID)
{
    VSOut o;

    uint sc = max(SliceCount, 1);
    float t = (sc == 1) ? 0.5f : (instanceId / (float) (sc - 1));
    o.sliceT = t;

    // AABB(HalfSizeWS) をカメラ基底に射影した半径へ
    float3 ar = abs(CamRightWS);
    float3 au = abs(CamUpWS);
    float3 af = abs(CamFwdWS);

    float extentR = dot(ar, HalfSizeWS);
    float extentU = dot(au, HalfSizeWS);
    float extentF = dot(af, HalfSizeWS);

    // スライス位置（カメラ前後方向）
    float z = lerp(-extentF, extentF, t);

    float3 ws =
        CenterWS
        + CamRightWS * (input.pos.x * extentR)
        + CamUpWS * (input.pos.y * extentU)
        + CamFwdWS * z;

    o.worldPos = ws;
    o.uv = input.uv;
    o.svpos = mul(float4(ws, 1.0f), ViewProj);
    return o;
}