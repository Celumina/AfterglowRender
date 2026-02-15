#include "TerrainCommon.hlsl"
#include "../Common.hlsl"
#include "../Meteorograph/MeteorographCommon.hlsl"

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	static const float meteorographSideRatio = float(meteorographSideElements) / terrainDataSideElements;
	
	// waterFlowDamping: To avoid water over oscillation.
	static const float waterFlowDamping = 0.005;
	static const float waterViscosity = 1.0;
	static const float soilDissolvingFactor = 0.1;
	static const float sedimentDepositingFactor = 0.025;
	static const float deltaTerrainDistance = 0.001;
	static const float hydrologicalCycleRate = 0.5;
	// Compensation factor from the meteorogragh resolution error.
	static const float evaporationFactor = 0.65;

	static const float thermalUnderwaterFactor = 0.1;
	static const float thermalWindIntensity = 0.1;
	static const float thermalHeatIntensity = 0.2;
	static const float thermalErosionRate = 0.2;

	static const uint windEffectWaveSize = 64;
	static const float windEffectIntensity = 0.65;


	static const float waterFlowPipeArea = terrainDataCellArea / waterViscosity;
	
	float2 worldPos = WorldPositionFromTileID(threadID.xy, terrainInvDataSideElements);

	// Fetch terrain data
	float2 terrainHeightIn = TerrainHeightIn[threadID.xy];
	float4 waterFlow = WaterFlowIn[threadID.xy];

	// Immediate variables for output.
	float terrainHeight = terrainHeightIn.x;
	float waterHeight = terrainHeightIn.y;
	// float waterSediment = waterHeightSediment.y;

	// Fetch meteorograph data
	uint2 meteorographID = uint2(threadID.xy * meteorographSideRatio);
	// float4 meteorograph = Meteorograph[meteorographID];
	// Sampling for smooth result.
	float4 meteorograph = SampleMeteorograph(Meteorograph, MeteorographSampler, worldPos);
	
	// Water Rainfall
	// float rainfall = Pluviometer[meteorographID].x;
	float rainfall = SampleMeteorograph(Pluviometer, PluviometerSampler, worldPos);
	waterHeight += max(0.0, rainfall * hydrologicalCycleRate);

	// Full Rain Test
	// if (frac(time * 0.2) > 0.99) {
	// 	waterHeight = terrainHeight + 0.2;
	// }
	
	// Fetch neighbours data (Cartesian Coords)
	// TODO: Gathering neighbour fetch methods.
	// uint2 idT = (threadID.xy + int2(0, 1)) % terrainDataSideElements;
	// uint2 idB = (threadID.xy + int2(0, -1)) % terrainDataSideElements;
	// uint2 idR = (threadID.xy + int2(1, 0)) % terrainDataSideElements;
	// uint2 idL = (threadID.xy + int2(-1, 0)) % terrainDataSideElements;
	uint2 idT = clamp(threadID.xy + int2(0, 1), 0, terrainDataSideElements - 1);
	uint2 idB = clamp(threadID.xy + int2(0, -1), 0, terrainDataSideElements - 1);
	uint2 idR = clamp(threadID.xy + int2(1, 0), 0, terrainDataSideElements - 1);
	uint2 idL = clamp(threadID.xy + int2(-1, 0), 0, terrainDataSideElements - 1);

	float2 terrainHeightT = TerrainHeightIn[idT];
	float2 terrainHeightB = TerrainHeightIn[idB];
	float2 terrainHeightR = TerrainHeightIn[idR];
	float2 terrainHeightL = TerrainHeightIn[idL];

	float4 neighbourSurfaceHeights = float4(
		max(terrainHeightT.x, terrainHeightT.y), 
		max(terrainHeightB.x, terrainHeightB.y), 
		max(terrainHeightR.x, terrainHeightR.y), 
		max(terrainHeightL.x, terrainHeightL.y)
	);
	
	// High deltaTime yields unstable simulation result, so slow down the simulation if framerate lower than 60 per second.
	float waterDeltaTime = min(deltaTime, 1.0 / 60.0);

	float surfaceHeight = max(terrainHeight, waterHeight);
	float4 surfaceDeltaHeights = surfaceHeight - neighbourSurfaceHeights;

	//  Wind affect to the water surface.
	if (all(threadID % windEffectWaveSize) == 0) {
		float4 maxDisturbance = max(neighbourSurfaceHeights - float4(terrainHeightT.x, terrainHeightB.x, terrainHeightR.x, terrainHeightL.x), 0.0) * waterDeltaTime;
		float2 resizedWind = ClampVectorWithLength(meteorograph.xy, 0.8, 64.0);
		float4 windEffect = float4(-resizedWind.x, resizedWind.x, -resizedWind.y, resizedWind.y);
		// @note: Here we use length(surfaceDeltaHeights) to cluster waves.
		surfaceDeltaHeights -= clamp(windEffect * windEffectIntensity * max(length(surfaceDeltaHeights), 0.001), -maxDisturbance, maxDisturbance);
	}

	// Shallow water equation with pipe model.	
	// Overflow (SWE)
	// Here we assume that the water has uniform density rho.
	// 	WaterFlow: (m^3)/s
	waterFlow = max(0.0, waterFlow + waterDeltaTime * waterFlowPipeArea * ((worldGravity * surfaceDeltaHeights) / terrainDataInterval));
	float idealFlow = (waterFlow.x + waterFlow.y + waterFlow.z + waterFlow.w);
	// Limit the max waterFlow never greater than the total water.
	float flowScalingFactor = min(
		1.0 - waterFlowDamping, (max(waterHeight - terrainHeight, 0.0) * (terrainDataInterval * terrainDataInterval)) / (idealFlow * waterDeltaTime)
	);
	waterFlow *= flowScalingFactor;
	WaterFlowOut[threadID.xy] = waterFlow;

	// Waiting for WaterFlow write in.
	GroupMemoryBarrierWithGroupSync();

	// InFlow
	float4 waterFlowT = WaterFlowOut[idT];
	float4 waterFlowB = WaterFlowOut[idB];
	float4 waterFlowR = WaterFlowOut[idR];
	float4 waterFlowL = WaterFlowOut[idL];

	// Component Order: T, B, R, L.
	float inFlow = waterFlowT.y + waterFlowB.x + waterFlowR.w + waterFlowL.z;
	float outFlow = waterFlow.x + waterFlow.y + waterFlow.z + waterFlow.w;
	float deltaWaterVolume = waterDeltaTime * (inFlow - outFlow);
	// Delta volume
	waterHeight += deltaWaterVolume * rcp(terrainDataInterval * terrainDataInterval);
	
	// Regenerate terrain normal
	float4 terrainNormal = CalculateTerrainNormal(terrainHeightIn, terrainHeightR, terrainHeightT);
	
	// Delta terrain height
	float4 deltaTerrainHeights = lerp(
		terrainHeightIn.xxxx, float4(terrainHeightR.x, terrainHeightL.x, terrainHeightT.x, terrainHeightB.x), deltaTerrainDistance
	);
	float2 partialTerrainHeightXY = (deltaTerrainHeights.xz - deltaTerrainHeights.yw) * (terrainInvDataInterval * (0.5 * (1.0 / deltaTerrainDistance))); 
	float terrainSlopeMagnitude = length(partialTerrainHeightXY);
	float terrainSlope = terrainSlopeMagnitude * rcp(sqrt(1.0 + terrainSlopeMagnitude * terrainSlopeMagnitude));

	// Sediment dissolution and deposition.
	// To avoid sediment in terrain surface
	if (waterHeight > terrainHeight) {
		// WaterVelocity: m/s
		// Flow limitation
		float avgWaterHeight = ((terrainHeightIn.y - terrainHeightIn.x, 0.0) + (waterHeight - terrainHeight)) * 0.5;
		float2 deltaWaterFlow = float2(
				((waterFlowL.z - waterFlow.w) + (waterFlow.z - waterFlowR.w)), 
				((waterFlowB.x - waterFlow.y) + (waterFlow.x - waterFlowT.y))
		) * 0.5;
		float2 waterVelocity = deltaWaterFlow / (max((abs(avgWaterHeight) * terrainDataInterval), 1e-1) * sign(avgWaterHeight)) * waterDeltaTime;

		// Sediment transportation (Semi-Lagrangian method)
		// Sediment is interporated from the source pos.
		// @fixed: Sediment seems in the same direction: sample uv should +(0.5 * terrainDataInterval) to get the center point.
		float sediment = SampleTerrain(
			WaterSedimentIn, WaterSedimentInSampler, worldPos + (0.5 * terrainDataInterval) - WaterVelocityIn[threadID.xy]
		);

		// float terrainSlope = length(terrainNormal.xy);

		// float2 partialTerrainHeightXY = float2(
		// 	terrainHeightR.x - terrainHeightL.x, 
		// 	terrainHeightT.x - terrainHeightB.x
		// ) * (0.5 * terrainInvDataInterval); 

		// if (abs(sediment) > 1e-6) {
		float sedimentCapability = waterSedimentCapability * length(waterVelocity) * terrainSlope;
		float deltaDissolvingSediment = (sedimentCapability - sediment);
		deltaDissolvingSediment *= deltaDissolvingSediment > 0.0 ? soilDissolvingFactor : sedimentDepositingFactor;
		terrainHeight -= deltaDissolvingSediment;
		sediment += deltaDissolvingSediment;
		WaterSedimentOut[threadID.xy] = sediment;
		WaterVelocityOut[threadID.xy] = waterVelocity;
		// }
		// else {
		// 	terrainHeight -= sediment;
		// 	WaterSedimentOut[threadID.xy].x = 0.0;
		// }

	}
	// else {
	// 	terrainHeight += WaterSedimentIn[threadID.xy].x;
	// 	WaterSedimentOut[threadID.xy] = 0.0;
	// 	WaterVelocityOut[threadID.xy] = 0.0;
	// }
	
	// Thermal Erosion
	float soilMoisture = SoilMoisture(waterHeight - terrainHeight);
	float thermalThreashold = 1.0;
	if (waterHeight > terrainHeight) { // Underwater
		thermalThreashold *= thermalUnderwaterFactor;	
	}
	else { // Climate
		// WindErosion
		thermalThreashold += max(dot(-meteorograph.xy, terrainNormal.xy) * (1.0 - soilMoisture), 0.0) * thermalWindIntensity;
		// HeatErosion
		thermalThreashold -= min(max(meteorograph.w - polarTemperature, 0.0) * (thermalHeatIntensity * 0.01), thermalHeatIntensity);
		thermalThreashold = saturate(thermalThreashold);
	}
	// Apply Thermal Erosion
	if (terrainSlope > thermalThreashold) {
		terrainHeight = lerp(
			terrainHeight, 
			(terrainHeightT.x + terrainHeightB.x + terrainHeightR.x + terrainHeightL.x) * 0.25, 
			saturate(waterDeltaTime * thermalErosionRate * (terrainSlope - thermalThreashold))
		);
	}
	
	// Water Evaporation.
	waterHeight -= DeltaEvaporation(length(meteorograph.xy), meteorograph.zw, soilMoisture, hydrologicalCycleRate * evaporationFactor);

	// Outputs
	// Balance sealevel from edge
	bool isEdgeCell = threadID.x >= (terrainDataSideElements - 1) 
		| threadID.y >= (terrainDataSideElements - 1) 
		| threadID.x <= 0
		| threadID.y <= 0;

	TerrainHeightOut[threadID.xy] = float2(terrainHeight, isEdgeCell ? max(waterHeight, waterBaseHeight) : waterHeight);
	TerrainNormalOut[threadID.xy] = terrainNormal;


	float waterToTerrainHeight = terrainHeight - waterHeight;
	half4 terrainSurface = 0.0;
	// .r Slope
	terrainSurface.r = smoothstep(0.3, 0.5, length(terrainNormal.xy));
	// .g Beach
	terrainSurface.g = max(1.0 - max((waterToTerrainHeight) * 10.0, 0.0), 0.0);
	// .b Humidity
	terrainSurface.b = max((waterToTerrainHeight - 0.5) * invMaxWaterSoilDiffuseDistance, 0.0);

	/*
	// Surface colors
	// Humidity
	float waterToTerrainHeight = terrainHeight - waterHeight;
	half4 terrainSurface = lerp(
		half4(0.15, 0.2, 0.04, 1.0), 
		half4(0.55, 0.4, 0.1, 1.0), 
		max((waterToTerrainHeight - 0.1) * invMaxWaterSoilDiffuseDistance, 0.0)
	);

	// Beach
	terrainSurface = lerp(
		terrainSurface, 
		half4(0.55, 0.52, 0.25, 1.0), 
		max(1.0 - max((waterToTerrainHeight) * 10.0, 0.0), 0.0)
	);

	// Slope
	terrainSurface = lerp(
		terrainSurface, 
		half4(0.25, 0.20, 0.15, 1.0), 
		smoothstep(0.3, 0.5, length(terrainNormal.xy))
	);

	// Snow
	terrainSurface = lerp(
		terrainSurface, 
		half4(0.7, 0.7, 0.72, 1.0), 
		saturate(-meteorograph.w * 0.5)
	);
	*/

	// use terrainSurface.a as roughness
	static const float surfaceRoughnessRange = 0.4;
	terrainSurface.a = lerp(
		0.95, 
		0.2, 
		-(1.0 / surfaceRoughnessRange) * min(abs(waterToTerrainHeight + 0.2) - surfaceRoughnessRange, 0.0)
	);
	

	TerrainSurfaceOut[threadID.xy] = terrainSurface;
}