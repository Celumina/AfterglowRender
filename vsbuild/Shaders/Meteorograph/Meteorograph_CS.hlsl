#include "../Random.hlsl"
#include "../Terrain/TerrainCommon.hlsl"
#include "MeteorographCommon.hlsl"

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	static const float humidityCapability = 1.0;
	static const float terrainSideRatio = float(terrainDataSideElements) / meteorographSideElements;
	static const float pressureWindMax = 0.005;
	static const float randomWindFieldIntensity = 0.2;
	static const float windSourceIntensity = 4.0;
	static const float diffuseBaseWeight = 0.01;
	
	float2 worldPos = WorldPositionFromTileID(threadID.xy, meteorographInvSideElements);

	// Fetch Terrain Data
	uint2 terrainID = threadID.xy * terrainSideRatio;
	float2 terrainHeight = TerrainHeight[terrainID];

	// Fetch meteorograph (Cartesian Coords)
	float4 meteorograph = MeteorographIn[threadID.xy];
	float4 meteorographT = MeteorographIn[(threadID.xy + int2(0, 1)) % meteorographSideElements];
	float4 meteorographB = MeteorographIn[(threadID.xy + int2(0, -1)) % meteorographSideElements];
	float4 meteorographR = MeteorographIn[(threadID.xy + int2(1, 0)) % meteorographSideElements];
	float4 meteorographL = MeteorographIn[(threadID.xy + int2(-1, 0)) % meteorographSideElements];

	// Wind iteration
	float deltaWindT = -meteorographT.y;
	float deltaWindB = meteorographB.y;
	float deltaWindR = -meteorographR.x;
	float deltaWindL = meteorographL.x;

	float4 deltaTemperature = float4(meteorographT.w, meteorographB.w, meteorographR.w, meteorographL.w) - meteorograph.w;
	float2 pressureWind = clamp(float2(deltaTemperature.z + deltaTemperature.w, deltaTemperature.x + deltaTemperature.y) * pressureWindMax, -pressureWindMax, pressureWindMax);

	float windSpeed = length(meteorograph.xy);

	// TODO: Include Navier-Stokes Equation to replace this.
	// WindSource: Random wind direction and intensity by time interpolation
	float3 windNoiseUV = float3(threadID.xy + time * float2(2.0, 0.0), time) * meteorographInvSideElements;
	float2 wind = lerp(meteorograph.xy, Snorm(float2(
		PerlinNoise(windNoiseUV, 10.0, randomSeed0), 
		PerlinNoise(windNoiseUV, 10.0, randomSeed1)
	)) * windSourceIntensity,  deltaTime * randomWindFieldIntensity);

	wind += (float2(deltaWindR + deltaWindL, deltaWindT + deltaWindB) + pressureWind) * deltaTime;

	float2 humitureFlowT = -meteorographT.y * meteorographT.zw;
	float2 humitureFlowB = meteorographB.y * meteorographB.zw;
	float2 humitureFlowR = -meteorographR.x * meteorographR.zw;
	float2 humitureFlowL = meteorographL.x * meteorographL.zw;

	// Solved: Different action in low and high framerate -> texture precision.
	// Transport
	float2 humiture = max(MeteorographIn[threadID.xy].zw + (humitureFlowT + humitureFlowB + humitureFlowR + humitureFlowL) * deltaTime, float2(0.0, absoluteZeroTemperature));
	// Diffuse
	float diffuseWeight = diffuseBaseWeight * deltaTime;
	float concentratedWeight = 1.0 - diffuseWeight * 4.0;
	humiture = humiture * concentratedWeight + (meteorographT.zw + meteorographB.zw + meteorographR.zw + meteorographL.zw) * diffuseWeight;

	// Evaporation and Rainfall
	float deltaWaterHeight = terrainHeight.y - terrainHeight.x;
	humiture.x += DeltaEvaporation(wind, humiture, SoilMoisture(deltaWaterHeight), 1.0);

	// Humidity saturation and rainfall
	float rainFall = max(0.0, humiture.x - humidityCapability) * deltaTime;
	humiture.x -= rainFall;

	// Temperature transport and recovery.
	humiture.y = lerp(humiture.y, SurfaceTemperature(worldPos.y, terrainHeight.x), deltaTime);
	// humiture.y = SurfaceTemperature(worldPos.y, terrainHeight.x);

	PluviometerOut[threadID.xy].x = rainFall;
	MeteorographOut[threadID.xy] = float4(wind, humiture);
}