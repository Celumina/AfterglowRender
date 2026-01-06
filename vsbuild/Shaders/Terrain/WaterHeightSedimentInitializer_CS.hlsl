#include "TerrainCommon.hlsl"

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	// R: Water Height
	// G: Water Sediment
	// float waterHeight = max(TerrainHeightOut[threadID.xy] - 10.0, waterBaseHeight);
	WaterHeightSedimentOut[threadID.xy] = float2(waterBaseHeight, 0.0);
}

