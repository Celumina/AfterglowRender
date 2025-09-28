[numthreads(256, 1, 1)]
void main(uint3 threadIDs : SV_DispatchThreadID) {
	uint index = threadIDs.x;
	ParticleSSBOStruct particleIn = ParticleSSBOIn[index];

	ParticleSSBOOut[index].position = particleIn.position + particleIn.velocity * deltaTime * 100.0;
	// ParticleSSBOOut[index].velocity = particleIn.velocity;

	const float3 boundaryMin = float3(-1000.0, -1000.0, 10.0);
	const float3 boundaryMax = float3(1000.0, 1000.0, 1000.0);

	if (ParticleSSBOOut[index].position.x < boundaryMin.x
		|| ParticleSSBOOut[index].position.x > boundaryMax.x) {
		ParticleSSBOOut[index].velocity.x *= -1;
	}
	
	if (ParticleSSBOOut[index].position.y < boundaryMin.y
		|| ParticleSSBOOut[index].position.y > boundaryMax.y) {
		ParticleSSBOOut[index].velocity.y *= -1;
	}

	if (ParticleSSBOOut[index].position.z < boundaryMin.z
		|| ParticleSSBOOut[index].position.z > boundaryMax.z) {
		ParticleSSBOOut[index].velocity.z *= -1;
	}
}