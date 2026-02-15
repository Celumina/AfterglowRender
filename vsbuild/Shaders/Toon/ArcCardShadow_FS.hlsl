#include "../VertexStructs.hlsl"

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(StandardFSInput input) {
	FSOutput output;

	output.color = float4(color.xyz, (1.0 - input.color.w) * color.w);
	return output;
}