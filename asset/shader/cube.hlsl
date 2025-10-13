struct ModelViewProjection
{
    matrix MVP;
};

cbuffer ModelViewProjectionCB : register(b0)
{
    matrix MVP;
};

struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Color : COLOR;
};

struct VertexShaderOutput
{
    float4 Color : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput VSMain(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(float4(IN.Position, 1.0f), MVP);
    OUT.Color = float4(IN.Color, 1.0f);

    return OUT;
}

struct PixelShaderInput
{
    float4 Color : COLOR;
};

float4 PSMain(PixelShaderInput IN) : SV_Target
{
    return IN.Color;
}