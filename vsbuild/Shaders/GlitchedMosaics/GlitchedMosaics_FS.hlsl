#include "../VertexStructs.hlsl"

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(UnlitFSInput input) {
	FSOutput output;	
	output.color = PresentIn.Sample(PresentInSampler, input.texCoord0, 0, 0.0);

	return output;
}