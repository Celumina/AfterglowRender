#include "../VertexStructs.hlsl"
#include "../Common.hlsl"

StandardInstancingVSOutput main(VSInput input) {
	// static const float shellFurInterval = 0.03;

	StandardInstancingVSOutput output;
	float3 position = input.position;

	position += input.normal * (input.instanceID + 1) * shellInterval;

	float4 worldPosition = mul(model, float4(position, 1.0));
	// @note: Good for demostration.
	// worldPosition.xyz += Unorm(sin(time)) * input.instanceID * 0.1;

	output.position = mul(viewProjection, worldPosition);
	output.worldPosition = worldPosition;
	output.worldNormal = normalize(mul(invTransModel, float4(input.normal, 0.0)).xyz);
	output.worldTangent = normalize(mul(invTransModel, float4(input.tangent, 0.0)).xyz);
	output.worldBitangent = cross(output.worldNormal, output.worldTangent);
	output.color = input.color;
	output.objectID = objectID;
	output.instanceID = input.instanceID;
	output.texCoord0 = input.texCoord0;
	return output;
}