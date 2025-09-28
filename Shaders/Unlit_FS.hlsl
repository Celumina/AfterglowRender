struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float3 worldNormal : NORMAL;
	[[vk::location(2)]] float4 color : COLOR;
	[[vk::location(3)]] float2 texCoord0 : TEXCOORD0;
};

struct PSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

PSOutput main(VSOutput input) {
	PSOutput output;	
	// tex.Sample(Sampler sampler, float2 uv, int2 offset[pixels], float lodBias)
	output.color = albedoTex.Sample(albedoTexSampler, input.texCoord0, 0, 0.0);

	return output;
}