#include "VertexStructs.hlsl"

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(UnlitFSInput input) {
	FSOutput output;	
	// tex.Sample(Sampler sampler, float2 uv, int2 offset[pixels], float lodBias)
	output.color = albedoTex.Sample(albedoTexSampler, input.texCoord0, 0, 0.0);

	return output;
}