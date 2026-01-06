#include "../Random.hlsl"

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	static const uint texSideElements = 256;
	// float2 uv = abs(threadID.xy - (texSideElements / 2)) * (0.5 / texSideElements);
	float2 uv = threadID.xy * (1.0 / texSideElements);

	float value = 0.0;
	static const uint fractalCount = 8;
	static const float invFractualCount = 1.0 / float(fractalCount);
	static const float baseScaling = 2.0;
	[Unroll] for(uint index = 0; index < fractalCount; ++index) {
		// TODO: Find a better epsilon model (weight).
		// {W = (0.5^i)} for octave weight; e.g. 1, 0.5, 0.25, 0.125...
		float weight = (2.0 * (fractalCount - index) / (fractalCount)) * invFractualCount;
		value += SimplexNoise(uv, pow(2.0, index + baseScaling), randomSeed0) * weight;
	}
	FractalNoiseOut[threadID.xy] = pow(value * 1.0, 2.0);
}