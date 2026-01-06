#ifndef WHIMSICAL_HLSL
#define WHIMSICAL_HLSL

/*
This File is used to record some whimsical ideas.
*/

/* Voxelized triangle vertex shader

StandardVSOutput main(VSInput input) {
	StandardVSOutput output;
	//// core code: truncation in object space
	// input.position = floor(input.position * 0.25) * 4.0;

	float4 worldPosition = mul(model, float4(input.position, 1.0));

	//// core code: truncation in world space
	worldPosition.xyz = floor(worldPosition.xyz * 0.25) * 4.0;

	output.position = mul(viewProjection, worldPosition);
	output.worldPosition = worldPosition;
	output.worldNormal = normalize(mul(invTransModel, float4(input.normal, 0.0)).xyz);
	output.worldTangent = normalize(mul(invTransModel, float4(input.tangent, 0.0)).xyz);
	output.worldBitangent = cross(output.worldNormal, output.worldTangent);
	output.color = input.color;
	output.objectID = objectID; // input.instanceID;
	output.texCoord0 = input.texCoord0;
	return output;
}

*/

#endif