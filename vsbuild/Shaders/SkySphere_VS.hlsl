#include "VertexStructs.hlsl"

UnlitVSOutput main(VSInput input) {
	UnlitVSOutput output;
	output.position = mul(viewProjection, mul(model, float4(input.position, 1.0)) + float4(cameraPosition.xyz, 0.0));
	output.texCoord0 = input.texCoord0;
	return output;
}