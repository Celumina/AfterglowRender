#include "MeteorographCommon.hlsl"
#include "../Random.hlsl"

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	// .rg: Wind velocity <m/s> (wind velocity z from delta location temperature)
	// .b: Humidity (RH) [0, 1]
	// .a: Temperature (Celsius) [-273.15, inf]
	float2 worldPos = WorldPositionFromTileID(threadID.xy, meteorographInvSideElements);
	float2 wind = Snorm(float2(
		PerlinNoise(float2(threadID.xy) * meteorographInvSideElements, 10.0, randomSeed0), 
		PerlinNoise(float2(threadID.xy) * meteorographInvSideElements, 10.0, randomSeed1)
	));
	float humidity = PerlinNoise(float2(threadID.xy) * meteorographInvSideElements, 20.0, randomSeed2);
	float temperature = SurfaceTemperature(worldPos.y, 0.0);

	MeteorographOut[threadID.xy] = float4(wind, humidity, temperature);
}