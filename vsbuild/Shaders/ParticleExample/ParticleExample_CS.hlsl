#include "../Random.hlsl"
#include "../Meteorograph/MeteorographCommon.hlsl"
#include "../Terrain/TerrainCommon.hlsl"

[numthreads(256, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	// TODO: Disapper in edge and generate randomly around the player.
	uint index = threadID.x;
	ParticleSSBOStruct particleIn = ParticleSSBOIn[index];

	static const float fadeThreshold = 0.25;
	static const float regeneratedRange = 8.0;
	static const float maxHorizontalSpeed = 0.5;
	static const float frictionFactor = 0.005;
	
	const float3 boundaryMin = float3(-10.0, -10.0, 0.0) + cameraPosition.xyz;
	const float3 boundaryMax = float3(10.0, 10.0, 10.0) + cameraPosition.xyz;

	float deltaElapseTime = deltaTime;
	float3 positionOut = ParticleSSBOOut[index].position;


	// If paticle leaving boundary, elapsing double time.
	if (any(positionOut < boundaryMin)
		|| any(positionOut > boundaryMax)) {
		deltaElapseTime *= 4;
	}
	
	ParticleSSBOOut[index].elapseTime = particleIn.elapseTime + deltaElapseTime;

	if (particleIn.elapseTime > particleIn.maxLifeTime) {
		ParticleSSBOOut[index].elapseTime = 0.0;
		// Generate higher than the ground and not in the upper air.
		positionOut = Hash3D(float3(index, index, index), randomSeed0);
		positionOut.xy = Snorm(positionOut.xy);
		float3 offset = { cameraPosition.xy , 0.0};
		positionOut = mad(positionOut, regeneratedRange, offset);

		float2 terrainHeight = LoadTerrain(TerrainHeight, positionOut);
		positionOut.z += terrainHeight.x;
	}
	else {
		positionOut = particleIn.position + particleIn.velocity * deltaTime * 1.0;
	}

	// Assume that the CS position is world position, ignore modelMat.
	// positionOut = mul(model, float4(positionOut, 1.0)).xyz;
	ParticleSSBOOut[index].position = positionOut;

	// Accumulate wind velocity.
	float3 velocityOut = particleIn.velocity;
	velocityOut.xy *= (1.0 - frictionFactor);
	velocityOut.xy += SampleMeteorograph(Meteorograph, MeteorographSampler, positionOut.xy).xy * deltaTime;
	velocityOut.xy = normalize(velocityOut.xy) * min(length(velocityOut.xy), maxHorizontalSpeed);
	ParticleSSBOOut[index].velocity = velocityOut;

	// Particle Opacity
	float fadeValue = min(min(particleIn.maxLifeTime - particleIn.elapseTime, particleIn.elapseTime) / particleIn.maxLifeTime, fadeThreshold);
	ParticleSSBOOut[index].color.a = fadeValue * (1.0 / fadeThreshold);

	// TODO: Global wind field
}