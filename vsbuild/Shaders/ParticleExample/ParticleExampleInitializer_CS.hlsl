#include "../Random.hlsl"

[numthreads(256, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	uint index = threadID.x;
	float invIndex = 1.0 / index;
	float colorSeed = randomSeed0 * 0.5;
	ParticleSSBOOut[index].position = Hash3D(float3(index, index, index), randomSeed0) * 1.0;
	ParticleSSBOOut[index].position.z += 1.0;
	ParticleSSBOOut[index].maxLifeTime = (Hash(invIndex, randomSeed1) + 1.0) * 10.0;
	ParticleSSBOOut[index].velocity = Snorm(Hash3D(ParticleSSBOOut[index].position * 100.0, randomSeed0)) * 0.25;
	ParticleSSBOOut[index].elapseTime = 0.0;
	ParticleSSBOOut[index].color = float4(Hash3D(float3(index, index, index), colorSeed), 1.0);

	TestTextureOut[uint2(index / 32, index % 32)].rg = abs(ParticleSSBOOut[index].velocity.xy) * 2.0;
}