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
};

VSOut main(VSIn input, uint instanceId : SV_InstanceID)
{
    VSOut o;

    uint sc = max(SliceCount, 1);
    float t = (sc == 1) ? 0.5f : (instanceId / (float) (sc - 1));
    o.sliceT = t;

    // カメラ基底の abs を使って、AABB(HalfSizeWS) を「カメラ方向に射影した半径」に変換
    float3 ar = abs(CamRightWS);
    float3 au = abs(CamUpWS);
    float3 af = abs(CamFwdWS);

    float halfW = dot(ar, HalfSizeWS);
    float halfH = dot(au, HalfSizeWS);
    float halfD = dot(af, HalfSizeWS);

    // 奥行き方向のスライス位置（-halfD ～ +halfD）
    float depth = lerp(-halfD, halfD, t);

    // 入力quadは [-1..1] の想定
    float2 q = input.pos.xy;

    float3 ws =
        CenterWS
        + CamRightWS * (q.x * halfW)
        + CamUpWS * (q.y * halfH)
        + CamFwdWS * depth;

    o.worldPos = ws;
    o.uv = input.uv;
    o.svpos = mul(float4(ws, 1.0f), ViewProj);
    return o;
}