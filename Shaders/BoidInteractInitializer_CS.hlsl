#include "Random.hlsl"

[numthreads(32, 1, 1)]
void main(uint3 threadIDs : SV_DispatchThreadID) {
	// 0 ~ 126 : Obstacles
	// 127 : Bait
	static const uint baitIndex = 127;
	uint index = threadIDs.x;

	// Initialize Obstacles
	// Translation: Spherical shape
	// Sphere center higher than the ground.
	static const float3 center = {0, 0, 100};
	float3 direction = Snorm(Hash3D(float3(index, index, index), randomSeed0));
	float radius = Hash(float(index), randomSeed0) * 400.0;
	InteractBufferOut[index].position = direction * radius;

	// Bait
	if (index == baitIndex) {
		InteractBufferOut[index].radius = 500.0;
	}

}