#include "../Random.hlsl"


[numthreads(16, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	// Parameters
	static const float channelOffset = 0.01;
	static const float gridScale = 64.0;
	static const float2 noiseScale = { 3, 18.0 };
	static const float mosaicThreshold = 0.55;
	static const float mosaicMaskThreshold = 0.65;

	// Normalize texture coord
	static const float2 uv = float2(threadID.xy) / 256.0;

	static const float invGridScale = 1.0 / gridScale;
	const float2 uvGrid0 = floor((uv) * gridScale) * invGridScale;  
	const float2 uvGrid1 = floor((uv + float2(channelOffset * 1.5, -channelOffset * 0.5)) * gridScale) * invGridScale;  
	const float2 uvGrid2 = floor((uv + float2(channelOffset * 0.5, channelOffset)) * gridScale) * invGridScale;  
	const float2 uvGrid3 = floor((uv + float2(-channelOffset * 2.0, channelOffset * 2.0)) * gridScale) * invGridScale;  

	float4 noise = {
		PerlinNoise(uvGrid0, noiseScale, randomSeed0), 
		PerlinNoise(uvGrid1, noiseScale, randomSeed1), 
		PerlinNoise(uvGrid2, noiseScale, randomSeed2), 
		PerlinNoise(uvGrid3, noiseScale, randomSeed3)
	};
	noise = noise > mosaicThreshold;

	MosaicNoiseOut[threadID.xy] = noise;

	float4 noiseMask = {
		PerlinNoise(uvGrid0, noiseScale, randomSeed4), 
		PerlinNoise(uvGrid1, noiseScale, randomSeed4 + 1.0), 
		PerlinNoise(uvGrid2, noiseScale, randomSeed4 + 2.0), 
		PerlinNoise(uvGrid3, noiseScale, randomSeed4 + 3.0)
	};
	// noiseMask = noiseMask > mosaicMaskThreshold;

	MosaicNoiseMaskOut[threadID.xy] = noiseMask * 1.35;
}