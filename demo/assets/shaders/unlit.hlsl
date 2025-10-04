struct VSInput {
    float3 pos : POSITION;
    float2 tex : TEXCOORD0;
    float3 norm : NORMAL;
};

struct v2p {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 norm : NORMAL;
};

cbuffer VertexUniforms : register(b0)
{
    float4x4 uMtxMVP;
    float4x4 uMtxNormal;
};

v2p vs_main(VSInput input) {
    v2p output;
    output.pos = mul(float4(input.pos, 1.0), uMtxMVP);
    output.norm = mul(float4(input.norm, 1.0), uMtxNormal);
    output.tex = input.tex;
    return output;
}

Texture2D uDiffuse : register(t0);
SamplerState uDiffuseSampler : register(s0);

float4 ps_main(v2p input) : SV_TARGET {
    return uDiffuse.Sample(uDiffuseSampler, input.tex);
}
