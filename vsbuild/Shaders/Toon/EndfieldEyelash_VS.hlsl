#include "EndfieldCommon.hlsl"
#include "../VertexStructs.hlsl"

StandardVSOutput main(VSInput input) {
	StandardVSOutput output;
	float4 worldPosition = mul(model, float4(input.position, 1.0));
	output.position = mul(viewProjection, worldPosition);

	// Stores original pixel depth.
	// Process data just like interal SV_POSITION do.
	// output.color.xy = float2(output.position.z, output.position.w);

	// Depth offset.
	output.position.z -= eyelashDepthOffset / output.position.w;

	output.worldPosition = worldPosition;
	output.worldNormal = normalize(mul(invTransModel, float4(input.normal, 0.0)).xyz);

	output.objectID = objectID; // input.instanceID;
	output.texCoord0 = input.texCoord0;
	return output;
}