#ifndef POST_EFFECT_UNDERWATER_HLSL
#define POST_EFFECT_UNDERWATER_HLSL

#include "Terrain/TerrainCommon.hlsl"

float UnderwaterMask(float2 uv, float softness = 0.01) {
	float4 nearWorldPos = mul(invViewProjection, float4(Snorm(uv), 0.0, 1.0));
	nearWorldPos.xyz /= nearWorldPos.w;
	float2 nearPlaneTerrainHeight = SampleTerrain(TerrainHeight, TerrainHeightSampler, nearWorldPos.xy);
	return smoothstep(0.0, softness, max(nearPlaneTerrainHeight.y - nearWorldPos.z + softness, 0.0));
}


#endif