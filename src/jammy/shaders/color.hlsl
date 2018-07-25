struct VsInput
{
	float2 pos : Position;
};

struct PsInput
{
	float4 pos : SV_Position;
};

struct PsOutput
{
	float4 color : SV_Target0;
};

cbuffer VsConstants : register(b0)
{
	float4x4 g_viewProjectionMatrix;
};

PsInput VertexMain(VsInput input)
{
	PsInput output;
	output.pos = mul(g_viewProjectionMatrix, float4(input.pos, 0, 1));
	return output;
}

cbuffer PsConstants : register(b0)
{
	float4 g_color;
};

PsOutput PixelMain(PsInput input)
{
	PsOutput output;
	output.color = g_color;
	return output;
}