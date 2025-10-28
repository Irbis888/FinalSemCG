#include "LightingUtil.hlsl"


Texture2D gHistory : register(t0);
Texture2D gCurrent : register(t1);
Texture2D gVelocity : register(t2);


SamplerState gsamLinearClamp : register(s3);

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
    
    vout.TexC = float2(vid & 1, (vid & 2) >> 1);
    vout.PosH = float4(vout.TexC * float2(4, -4) + float2(-1, 1), 0, 1);

    return vout;
}
struct PSOutput
{
    float4 RT0 : SV_Target0;
    //float4 RT1 : SV_Target1;
};


PSOutput PS(VertexOut pin)
{
    float4 currentColor = gCurrent.Load(int3(pin.PosH.xyz));
    float4 historyColor = gHistory.Load(int3(pin.PosH.xyz));
    float3 velocity = gVelocity.Load(int3(pin.PosH.xyz));
    float alpha = 0.1;
    float4 finalColor = alpha * currentColor + (1.0 - alpha) * historyColor;
    PSOutput output;
    output.RT0 = finalColor;
    //output.RT1 = finalColor;
    return output;
}
