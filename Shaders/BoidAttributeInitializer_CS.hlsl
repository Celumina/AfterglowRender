#include "Random.hlsl"

[numthreads(256, 1, 1)]
void main(uint3 threadIDs : SV_DispatchThreadID) {
	uint index = threadIDs.x;

	// Initialize boid buffer
	BoidBufferOut[index].acceleration = Snorm(Hash3D(float3(index, index, index), randomSeed0 - 104.351)) * 0.1;
	BoidBufferOut[index].influenceRadius = (Hash(index, randomSeed0 + 0.5)) * 5.0;
	float3 maxAcceleration = (Hash3D(float3(index, index, index), randomSeed0 + 34.124) + 4.0) * 0.1;
	BoidBufferOut[index].maxAcceleration = maxAcceleration;
	BoidBufferOut[index].turnSpeed = (UniformBitHash(index + uint(maxAcceleration.x)) + 4.0) * 0.1;
	BoidBufferOut[index].velocity = float3(0.0, 0.0, 0.0);
	BoidBufferOut[index].curiosity = (Hash(index, randomSeed1) + 0.2) * 0.1;
	BoidBufferOut[index].maxVelocity = (Hash3D(maxAcceleration, randomSeed1) + 4.0) * 0.5;
	BoidBufferOut[index].perception = (Hash(BoidBufferOut[index].curiosity, randomSeed0) + 0.5) * 200;
}