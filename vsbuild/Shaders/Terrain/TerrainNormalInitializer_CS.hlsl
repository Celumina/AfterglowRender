#include "TerrainCommon.hlsl"

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	float2 height = TerrainHeightOut[threadID.xy];
	float2 heightR = TerrainHeightOut[min(threadID.xy + uint2(1, 0), terrainDataSideElements)];
	float2 heightT = TerrainHeightOut[min(threadID.xy + uint2(1, 0), terrainDataSideElements)];
	TerrainNormalOut[threadID.xy] = calculateTerrainNormal(height, heightR, heightT);

	// Another method:
	// uint2 indexR = min(threadID.xy + uint2(1, 0), terrainDataSideElements);
	// uint2 indexD = min(threadID.xy + uint2(0, 1), terrainDataSideElements);
	// float3 position = float3(terrainDataInterval * threadID.xy, TerrainHeightIn[threadID.xy].x);
	// float3 positionR = float3(terrainDataInterval * indexR, TerrainHeightIn[indexR].x);
	// float3 positionD = float3(terrainDataInterval * indexD, TerrainHeightIn[indexD].x);
	// float3 normal = normalize(cross(positionR - position, positionD - position));
	// TerrainNormalOut[threadID.xy].xy = normal.xy;
}