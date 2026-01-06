#ifndef METEOROGRAPH_HLSL
#define METEOROGRAPH_HLSL


#include "../WorldDefinitions.hlsl"
#include "../Constants.hlsl"

static const uint meteorographSideElements = 128;

static const float meteorographInvSideElements = 1.0 / meteorographSideElements;
static const float meteorographInterval = worldSideLength / meteorographSideElements;
static const float meteorographInvInterval = 1.0 / meteorographInterval;

static const float absoluteZeroTemperature = -273.15; // Celsius
static const float polarTemperature = -10.0;
static const float equatorialTemperature = 30.0; 
static const float equatorialEvaporationRate = 1.2e-4f; // 1.2e-8f; // kg/((m^2)*Pa*(m^-1)*s)
static const float environmentalLapseRate = -0.325; // In real world, this value(ELR) is -0.0065 k/m
static const float baseAirConvection = 0.2;
static const float baseAirResistance = 1.0e-4; // In speed == 1; Resistance ∝ speed^2.

static const float maxWaterSoilDiffuseDistance = 1.0;
static const float soilEvaporationFactor = 0.2;
static const float invMaxWaterSoilDiffuseDistance = 1.0 / maxWaterSoilDiffuseDistance;


float AirResistance(float speed) {
	return baseAirResistance * (speed * speed);
}

// TODO: Time of Day support.
/**
* @desc: Assume that the world was wrapped as a sphere.
* @return: cos(SolarElevationAngle(radians)) at noon.
*/
float SunlightRatio(float worldPosY) {
	float y = abs(worldPosY * worldInvSideLength * 2.0);
	return dot(float2(1.0, 0.0), float2(1.0 - y, y));
}

/**
* @param theta: Angle between terrain(marcoslope) and the Sun.
*/
float SurfaceTemperature(float worldPosY, float terrainHeight) {
	float seaLevelTemperature = lerp(
		polarTemperature, 
		equatorialTemperature, 
		SunlightRatio(worldPosY)
	);
	return max(seaLevelTemperature + terrainHeight * environmentalLapseRate, absoluteZeroTemperature);
}

/*
* @param deltaWaterHeight: TerrainHeight - WaterHeight
* 	if deltaWaterHeight >= 0, then the moisure factor == 1; 
* 	if deltaWaterHeight < 0, then the moisure factor will fade off until zero.
*/
float SoilMoisture(float deltaWaterHeight) {
	// Assume that moisture surface from a soil cube
	// return soilEvaporationFactor - pow(clamp(-deltaWaterHeight * (invMaxWaterSoilDiffuseDistance * soilEvaporationFactor), 0.0, soilEvaporationFactor), 2.0 / 3.0);
	// Linear falloff for performance
	return soilEvaporationFactor - clamp(-deltaWaterHeight * (invMaxWaterSoilDiffuseDistance * soilEvaporationFactor), 0.0, soilEvaporationFactor);
}

/**
* @brief: A mixture emprical model for evaporation.
* @param windSpeed: <m/s>
* @param humitrue: .x is RelativeHumidity(RH) [0, 1] and .y is temperature(Celsius).
* @param soilMoisture: Evaluated by soilMoisture() uses delta water height with terrain. 
* @param area: for with different area size:
*	- terrain sampling: meteorographSideElements / terrainSideElements
* 	- meteorograph sampling: 1.0
* @return: Delta Evaporation (kg/((m^2)*s))
*/ 
float DeltaEvaporation(float windSpeed, float2 humiture, float soilMoisture, float area) {
	// Saturation vapor pressure(Pa)
	float clampedTemperature = clamp(humiture.y, 0.0, 100.0);
	float vaporPressure = 610.78 * exp((17.27 * clampedTemperature) / (clampedTemperature + 237.3));

	// Vapor pressure deficit (SaturationVaporPressure - ActualVaporPressure)
	float vpd = max(vaporPressure - humiture.x * vaporPressure, 0.0);
	
	// TODO: Limit maximum deltaTime to avoid some unexcepted pause e.g. breakpoint.
	return equatorialEvaporationRate * max(windSpeed, baseAirConvection) * vpd * soilMoisture * area * deltaTime;
}

template<typename ElementType>
ElementType LoadMeteorograph(Texture2D<ElementType> data, in float2 worldPosition) {
	// Clamp in the edges.
	uint2 dataIndex = clamp((worldPosition - worldCenterOffset) * meteorographInvInterval, 0, meteorographSideElements);
	return data.Load(uint3(dataIndex, 0));
}

template<typename ElementType>
ElementType SampleMeteorograph(Texture2D<ElementType> data, SamplerState dataSampler,  in float2 worldPosition) {
	float2 uv = clamp((worldPosition - worldCenterOffset) * (meteorographInvInterval * meteorographInvSideElements), 0.0, 1.0);
	return data.SampleLevel(dataSampler, uv, 0);
}


#endif