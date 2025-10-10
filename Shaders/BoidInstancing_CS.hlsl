#include "Random.hlsl"

[numthreads(256, 1, 1)]
void main(uint3 threadIDs : SV_DispatchThreadID) {
	static const uint boidCount = 1024;

	uint index = threadIDs.x;
	BoidBufferStruct boid = BoidBufferIn[index];
	InstanceBufferStruct transform = InstanceBufferIn[index];
	float3 position = float3(transform.transformR0.w, transform.transformR1.w, transform.transformR2.w);
	float3 acceleration = boid.acceleration;

	// influenceWeight < 0.5: Attraction
	// influenceWeight > 0.5: Repulsion
	const static float attractiveThreshold = 0.5;
	
	for (uint otherBoidIndex = 0; otherBoidIndex < boidCount ; ++otherBoidIndex) {
		if (otherBoidIndex == index) {
			continue;
		}
		InstanceBufferStruct otherBoidTransform = InstanceBufferIn[otherBoidIndex];
		float3 otherBoidPosition = float3(
			otherBoidTransform.transformR0.w, otherBoidTransform.transformR1.w, otherBoidTransform.transformR2.w
		);
		float boidDistance = distance(position, otherBoidPosition);
		if (boidDistance > boid.perception) {
			continue;
		}
		BoidBufferStruct otherBoid = BoidBufferIn[otherBoidIndex];
		float influence = (otherBoid.influenceRadius + boid.influenceRadius) - boidDistance;
		if (influence <= 0.0) {
			continue;
		}
		// Inversed square attenuation
		float influenceWeight = Square(influence / boidDistance);

		// Apply weighted acceleration
		
		float3 toOtherBoid = otherBoidPosition - position;
		float3 toOtherBoidDir = normalize(toOtherBoid);
		float3 maxAccelerationToOtherBoid = boid.maxAcceleration * toOtherBoidDir;
		float3 attractiveAccelration = lerp(
			0.0, maxAccelerationToOtherBoid, max(1.0 - influenceWeight - attractiveThreshold, 0.0)
		);
		float3 repulsiveAcceleration = lerp(
			0.0, -maxAccelerationToOtherBoid, max(influenceWeight - attractiveThreshold, 0.0)
		);
		acceleration += attractiveAccelration + repulsiveAcceleration;
	}

	// Update velocity
	acceleration = clamp(-boid.maxAcceleration, boid.maxAcceleration, acceleration);
	float3 velocity = clamp(-boid.maxVelocity, boid.maxVelocity, boid.velocity + acceleration);
	BoidBufferOut[index].velocity = velocity;

	// Acceleration attenuation
	acceleration += normalize(-acceleration) * 0.01;
	BoidBufferOut[index].acceleration = acceleration;

	// Process Orientation


	// Apply to transform
	position += velocity;
	InstanceBufferOut[index].transformR0.w = position.x;
	InstanceBufferOut[index].transformR1.w = position.y;
	InstanceBufferOut[index].transformR2.w = position.z;
}