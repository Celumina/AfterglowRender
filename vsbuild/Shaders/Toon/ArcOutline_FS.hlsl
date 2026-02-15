#include "../VertexStructs.hlsl"

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(UnlitFSInput input) {
	FSOutput output;	
	output.color = float4(0.0, 0.0, 0.0, 1.0);

	return output;
}