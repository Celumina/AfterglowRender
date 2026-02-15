#include "../VertexStructs.hlsl"

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(UnlitFSInput input) {
	FSOutput output;
	output.color.xyz = 0.0;
	output.color.a = 1.0 - input.texCoord0.y;
	return output;
}