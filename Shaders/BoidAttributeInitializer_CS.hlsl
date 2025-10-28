#include "Random.hlsl"

[numthreads(256, 1, 1)]
void main(uint3 threadIDs : SV_DispatchThreadID) {
	uint index = threadIDs.x;

	// Initialize boid buffer
	BoidBufferOut[index].acceleration = Snorm(Hash3D(float3(index, index, index), randomSeed0 - 104.351)) * 0.5;
	BoidBufferOut[index].influenceRadius = (Hash(index, randomSeed0) + 2.0) * 1.5;
	float3 maxAcceleration = (Hash3D(1.0 / float3(index, index, index), randomSeed0 + 34.124) + 8.0) * 0.02;
	BoidBufferOut[index].maxAcceleration = maxAcceleration;
	// TODO: Replace to scared for boid alert support
	BoidBufferOut[index].turnSpeed = (UniformBitHash(index) + 2.0) * 0.01; 
	BoidBufferOut[index].velocity = float3(0.0, 0.0, 0.0);
	BoidBufferOut[index].curiosity = saturate((Hash(index, randomSeed1) + 0.2) * 0.25);
	BoidBufferOut[index].maxVelocity = (Hash3D(maxAcceleration, randomSeed1) + 8.0) * 1.0;
	BoidBufferOut[index].perception = (Hash(BoidBufferOut[index].curiosity, randomSeed0) + 0.2) * 20;
}