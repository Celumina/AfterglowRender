#include "../VertexStructs.hlsl"

StandardVSOutput main(VSInput input) {
	StandardVSOutput output;
	float4 worldPosition = mul(model, float4(input.position, 1.0));	

	output.position = mul(viewProjection, worldPosition);
	output.worldPosition = worldPosition;
	output.worldNormal = normalize(mul(invTransModel, float4(input.normal, 0.0)).xyz);
	output.worldTangent = normalize(mul(invTransModel, float4(input.tangent, 0.0)).xyz);
	output.worldBitangent = cross(output.worldNormal, output.worldTangent);
	
	// color stores normalized object space position.
	float3 normalizePosition = (input.position.xyz - minAABB) / (maxAABB - minAABB);
	output.color.xyz = normalizePosition;

	output.objectID = objectID; // input.instanceID;
	output.texCoord0 = input.texCoord0;
	return output;
}