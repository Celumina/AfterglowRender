#include "GrassCommon.hlsl"
#include "../Random.hlsl"
#include "../Terrain/TerrainCommon.hlsl"
#include "../Meteorograph/MeteorographCommon.hlsl"

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	// .r: Grass Terrain Possibility 
	// .g: Grass Hash
	// .b: Grass Scaling
	// .a: Unused

	// Terrain data
	float2 worldPos = WorldPositionFromTileID(threadID.xy, grassInvDataSideElements); 
	float2 terrainHeight = LoadTerrain(TerrainHeight, worldPos);

	float grassPossibility = 0.0;
	if (terrainHeight.x > terrainHeight.y + 1e-1) {
		float2 terrainHeightT = LoadTerrain(TerrainHeight, worldPos + float2(terrainDataInterval, 0.0));
		float2 terrainHeightR = LoadTerrain(TerrainHeight, worldPos + float2(0.0, terrainDataInterval));
		float4 terrainNormal = calculateTerrainNormal(terrainHeight, terrainHeightR, terrainHeightT);

		float slopeFactor = min(length(terrainNormal.xy) * 2.0, 1.0);
		// grassPossibility = 1.0 - max(aridityFactor, slopeFactor);
		grassPossibility = 1.0 - slopeFactor;
	}
	
	float grassHash = Hash(float(threadID.x * grassInvDataSideElements + threadID.y * (grassInvDataSideElements * grassInvDataSideElements)), randomSeed0) * 0.5;

	float aridityFactor = clamp((terrainHeight.x - terrainHeight.y) / maxWaterSoilDiffuseDistance, 0.0, 1.0);
	float grassScaling = lerp(0.8, 0.2, aridityFactor);

 	GrassDataOut[threadID.xy] = float4(grassPossibility , grassHash, grassScaling, 0.0);
}