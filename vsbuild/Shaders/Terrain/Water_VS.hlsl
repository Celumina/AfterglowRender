#include "TerrainCommon.hlsl"
#include "../Common.hlsl"

struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float4 worldPosition : POSITION;
};

VSOutput main(VSInput input) {
	VSOutput output;

	float2 globalOffset = floor(cameraPosition.xy * (1 / waterMeshInterval)) * waterMeshInterval;
	input.position.xy += globalOffset;

	input.position.z = LoadTerrain(TerrainHeight, input.position.xy).y;

	output.worldPosition = mul(model, float4(input.position, 1.0));
	output.position = mul(viewProjection, output.worldPosition);
		
	return output;
}