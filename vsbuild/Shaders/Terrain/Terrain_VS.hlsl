#include "TerrainCommon.hlsl"
#include "../Common.hlsl"

struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float4 worldPosition : POSITION;
};

VSOutput main(VSInput input) {
	VSOutput output;

	float2 globalOffset = floor(cameraPosition.xy * (1 / terrainMeshInterval)) * terrainMeshInterval;
	input.position.xy += globalOffset;

	input.position.z = LoadTerrain(TerrainHeight, input.position.xy).x;

	output.worldPosition = mul(model, float4(input.position, 1.0));
	output.position = mul(viewProjection, output.worldPosition);

	// float2 terrainNormal = LoadTerrain(TerrainNormal, input.position.xy).xy;
	// input.normal = ReconstructNormal(terrainNormal);
	// output.worldNormal = mul(invTransModel, float4(input.normal, 0.0)).xyz;
	
	return output;
}