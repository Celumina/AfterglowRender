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

	static const float maxGrassScaling = 1.6;
	static const float minGrassScaling = 0.4;

	// Terrain data
	float2 worldPos = WorldPositionFromTileID(threadID.xy, grassInvDataSideElements); 
	float2 terrainHeight = LoadTerrain(TerrainHeight, worldPos);
	
	// @note: Gathering test.
	// float2 uv = clamp((worldPos - worldCenterOffset) * (terrainInvDataInterval * terrainInvDataSideElements), 0.0, 1.0);
	// float4 nr =  TerrainNormal.GatherRed(TerrainNormalSampler, uv);
	// float4 ng =  TerrainNormal.GatherGreen(TerrainNormalSampler, uv);
	// float rmax = max(max(nr.x, nr.y), max(nr.z, nr.w));
	// float gmax = max(max(ng.x, ng.y), max(ng.z, ng.w));
	// float2 terrainNormal = float2(rmax, gmax);

	float grassPossibility = 0.0;
	if (terrainHeight.x > terrainHeight.y + 1e-1) {
		float2 terrainHeightT = LoadTerrain(TerrainHeight, worldPos + float2(terrainDataInterval, 0.0));
		float2 terrainHeightR = LoadTerrain(TerrainHeight, worldPos + float2(0.0, terrainDataInterval));
		float4 terrainNormal = CalculateTerrainNormal(terrainHeight, terrainHeightR, terrainHeightT);

		float slopeFactor = min(length(terrainNormal.xy) * 2.0, 1.0);
		// grassPossibility = 1.0 - max(aridityFactor, slopeFactor);
		grassPossibility = 1.0 - slopeFactor;
	}
	
	float grassHash = Hash(float(threadID.x * grassInvDataSideElements + threadID.y * (grassInvDataSideElements * grassInvDataSideElements)), randomSeed0) * 0.5;

	float aridityFactor = clamp((terrainHeight.x - terrainHeight.y) / maxWaterSoilDiffuseDistance, 0.0, 1.0);
	float grassScaling = lerp(maxGrassScaling, minGrassScaling, aridityFactor);

 	GrassDataOut[threadID.xy] = float4(grassPossibility , grassHash, grassScaling, 0.0);
}