#include "Random.hlsl"

[numthreads(256, 1, 1)]
void main(uint3 threadIDs : SV_DispatchThreadID) {
	uint index = threadIDs.x;
	float colorSeed = randomSeed0 * 0.5;
	ParticleSSBOOut[index].position.xyz = Hash3D(float3(index, index, index), randomSeed0) * 100.0;
	ParticleSSBOOut[index].position.z += 100.0;
	ParticleSSBOOut[index].velocity.xyz = Snorm(Hash3D(ParticleSSBOOut[index].position, randomSeed0)) * 0.5;
	ParticleSSBOOut[index].color = float4(Hash3D(float3(index, index, index), colorSeed), colorSeed);
}