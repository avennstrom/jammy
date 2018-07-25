struct VsInput
{
	float2 pos : Position;
	float2 uv : Texcoord;
};

struct PsInput
{
	float4 pos : SV_Position;
	float2 uv : Texcoord;
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
	output.uv = input.uv;
	return output;
}

cbuffer PsConstants : register(b0)
{
	float4 g_color;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PsOutput PixelMain(PsInput input)
{
	const float4 texColor = g_texture.Sample(g_sampler, input.uv);
	clip(texColor.a ? 1 : -1);

	PsOutput output;
	output.color = texColor * g_color;
	return output;
}