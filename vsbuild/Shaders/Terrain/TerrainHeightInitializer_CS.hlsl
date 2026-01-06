#include "../random.hlsl"
#include "TerrainCommon.hlsl"


[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	// .x: TerrainHeight
	// .y: WaterDeltaHeight

	float2 uv = threadID.xy * terrainInvDataSideElements;
	// Terrain parameters (fractal Brownian motion based)
	// Steep
	static const float flattness = 0.25;
	static const float steepMaskScaling = 20.0;

	// Disturbance
	static const float regularity = 0.4;
	static const float disturbanceIntensityScaling = 10.0;
	static const float disturbanceFactor = 0.025;
	static const float disturbanceIntensityMin = 0.2;
	static const float disturbanceScaling = 200.0;

	// Base Terrain
	static const float baseTerrainScaling = 50.0;
	static const float baseTerrainHeightFactor = 80.0;
	static const float bsseTerrainBias = 4.0;

	// Fractal Brownian Motion
	static const uint fractalCount = 8;
	static const float minFractalHeightWeight = 0.1;
	static const float fractalHeightWeightIntensity = 4.0;

	// Rift
	static const float riftLimit = 0.4;
	static const float riftMaskScaling = 40.0;
	static const float riftScaling = 80.0;
	static const float riftDisturbanceIntensity = 2.0;
	static const float riftDepth = 10.0;
	static const float riftSoftness = 0.2;
	static const float riftWidth = 0.1;

	// Ridge
	static const float ridgeScaling = 30.0;
	static const float ridgeMaskScaling = 10.0;
	static const float ridgeSoftness = 0.15;
	static const float ridgeTransparency = 0.2;
	static const float ridgeLimit = 0.25;
	static const float ridgeAmplitude = 60.0;

	// Trend
	static const float trendScaling = 2.0;
	static const float trendAmplitude = 80.0;

	float steep = smoothstep(0.0, 1.0 - flattness, (PerlinNoise(uv, steepMaskScaling, randomSeed0) - flattness)); 
	float disturbanceIntensity = max((PerlinNoise(uv, disturbanceIntensityScaling, randomSeed0 + 1.0) - regularity), disturbanceIntensityMin) * disturbanceFactor;
	float2 disturbance = float2(
		PerlinNoise(uv, disturbanceScaling, randomSeed1), 
		PerlinNoise(uv, disturbanceScaling, randomSeed2) 
	) * disturbanceIntensity;

	// Base Terrain
	float terrainHeight = (WorleyNoise(uv + disturbance, baseTerrainScaling, randomSeed0)) * baseTerrainHeightFactor * steep + bsseTerrainBias;

	// Fractal Brownian Motion
	float fractalWeight = 1.0;
	float fractalScaling = 2.0;
	const float fractalHeightWeight = (steep + minFractalHeightWeight) * fractalHeightWeightIntensity;
	[Unroll] for(uint index = 0; index < fractalCount; ++index) {
		fractalWeight *= 0.5;
		fractalScaling *= 2.0;
		terrainHeight += Snorm(SimplexNoise(uv, fractalScaling, randomSeed0)) * fractalWeight * baseTerrainScaling * fractalHeightWeight;
	}

	// Rift in the plain(form worley stride)
	// TOOD: ...Use exponential value to present sharpness
	float riftMask = smoothstep(0.0, riftSoftness, (PerlinNoise(uv + disturbance * riftDisturbanceIntensity, riftMaskScaling, randomSeed1) - riftLimit)); 
	float riftHeight = -smoothstep(0.0, riftSoftness, riftWidth - abs(Snorm(PerlinNoise(uv + disturbance * riftDisturbanceIntensity, riftScaling, randomSeed0 + 1.0)))) * riftDepth;
	riftHeight *= max(0.4 - steep, 0.0) * riftMask;
	terrainHeight += riftHeight;

	// Ridge
	float ridge = (0.5 - abs(Snorm(PerlinNoise(uv + disturbance * 0.5, ridgeScaling, randomSeed1)))) * ridgeAmplitude;
	float ridgeMask = max(smoothstep(0.0, ridgeSoftness, (PerlinNoise(uv, ridgeMaskScaling, randomSeed3) - ridgeLimit) * steep) - ridgeTransparency, 0.0);
	terrainHeight = lerp(terrainHeight, ridge, ridgeMask);

	// Trend
	terrainHeight += Snorm(PerlinNoise(uv, trendScaling, randomSeed0)) * trendAmplitude;

	// Center platform
	float2 worldPos = WorldPositionFromTileID(threadID.xy, terrainInvDataSideElements);
	terrainHeight = lerp(0.0, terrainHeight, saturate(length(worldPos) * 0.01));

	// TODO: Terrain physical parameters
	// Solidity
	// 

	TerrainHeightOut[threadID.xy].xy = float2(terrainHeight, max(waterBaseHeight, terrainHeight - 0.25)); 
	// TerrainHeightOut[threadID.xy].xy = float2(0.0, 1.0);
}