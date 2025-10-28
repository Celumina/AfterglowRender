#include "Random.hlsl"
#include "Constants.hlsl"


struct BoidDistanceInfo {
	float3 position;
	float distance;
	uint index;
};

struct BoidAccumulationInfo {
	float3 position;
	uint numBoids;
	float3 acceleration;
};

[numthreads(256, 1, 1)]
void main(uint3 threadIDs : SV_DispatchThreadID) {
	static const uint boidCount = 1024;
	// Temporary: If boid lost its target, go back to here.
	static const float3 homePos = float3(sin(time * 0.2) * 100.0, cos(time * 0.2) * 100.0, 50.0);

	uint index = threadIDs.x;
	BoidBufferStruct boid = BoidBufferIn[index];
	InstanceBufferStruct instanceInfo = InstanceBufferIn[index];
	float3 position = float3(
		instanceInfo.instanceModel[0].w, 
		instanceInfo.instanceModel[1].w, 
		instanceInfo.instanceModel[2].w
	);
	float3 acceleration = boid.acceleration;
	float rcpPerception = rcp(boid.perception);
	// The exponent smaller, the flock size larger.
	float alignedAccelerationWeight = pow(rcpPerception, 0.75);

	BoidDistanceInfo closestBoidInfo = {float3(0.0, 0.0, 0.0), 1e+16f, index};
	BoidAccumulationInfo boidAccumulationInfo = {float3(0.0, 0.0, 0.0), 0, float3(0.0, 0.0, 0.0)};
	
	for (uint otherBoidIndex = 0; otherBoidIndex < boidCount ; ++otherBoidIndex) {
		if (otherBoidIndex == index) {
			continue;
		}
		InstanceBufferStruct otherBoidInstanceInfo = InstanceBufferIn[otherBoidIndex];
		BoidDistanceInfo otherBoidInfo;
		otherBoidInfo.position = float3(
			otherBoidInstanceInfo.instanceModel[0].w, 
			otherBoidInstanceInfo.instanceModel[1].w, 
			otherBoidInstanceInfo.instanceModel[2].w
		);
		otherBoidInfo.distance = distance(position, otherBoidInfo.position);
		otherBoidInfo.index = otherBoidIndex;

		// Use Perception to accumulate position, for a large gathering magnitude. 
		if (otherBoidInfo.distance <= boid.perception) {
			// boidAccumulationInfo.position += otherBoidInfo.position;
			boidAccumulationInfo.position += lerp(position, otherBoidInfo.position, rcpPerception);
			++boidAccumulationInfo.numBoids;
			boidAccumulationInfo.acceleration += lerp(acceleration, BoidBufferIn[otherBoidIndex].acceleration, alignedAccelerationWeight);
			// TODO: Accumulated Acceleration?
		}

		if (otherBoidInfo.distance < closestBoidInfo.distance) {
			closestBoidInfo = otherBoidInfo;
		}
	}

	// If have not any other boid in perception, back to the origin.
	float rcpNumBoids = rcp(boidAccumulationInfo.numBoids);
	float3 toCenter = boidAccumulationInfo.numBoids > 0 ? boidAccumulationInfo.position * rcpNumBoids - position : homePos - position;
	float3 toCenterDir = normalize(toCenter);
	float3 maxAccelerationToCenter = boid.maxAcceleration * toCenterDir;

	BoidBufferStruct closestBoid = BoidBufferIn[closestBoidInfo.index];
	float3 toClosestBoid = closestBoidInfo.position - position;
	float3 toClosestBoidDir = normalize(toClosestBoid);
	float3 maxAccelerationToClosestBoid = boid.maxAcceleration * toClosestBoidDir;

	float closestInfluence = (closestBoid.influenceRadius + boid.influenceRadius) - closestBoidInfo.distance;
	float closestInfluenceWeight = Square(max(closestInfluence, 0.0) / closestBoidInfo.distance);

	// P2: Matching closest boids velocity
	float3 alignedAcceleration = (boidAccumulationInfo.numBoids > 0 ? boidAccumulationInfo.acceleration * rcpNumBoids : acceleration);
	acceleration = lerp(alignedAcceleration, acceleration, boid.curiosity);
	// acceleration = lerp(acceleration, closestBoid.acceleration, 1.0 - boid.curiosity);

	// Action Chase: Close to other boid directly.
	// Action Flocking: Clost to boids center.

	// P0, P1...: The priority of the action, smaller first.
	// If influenceWeight greater than zero, boid will *P0: Avoid collision* insstead of *P1: Flock centering*.

	// Steering force, instead of inverse acceleration.
	float3 steeringAcceleration = normalize(toClosestBoidDir + toCenterDir) * boid.maxAcceleration;
	acceleration += closestInfluenceWeight == 0.0 ? maxAccelerationToCenter * 0.01 : steeringAcceleration * boid.turnSpeed;
	acceleration -= maxAccelerationToClosestBoid * closestInfluenceWeight;
	// Temporary: Pull up from earth, use terrain buffer intead.
	acceleration.z += max(20.0 - position.z, 0.0) * 0.05 * boid.maxAcceleration.z;

	// Update velocity
	float3 chaseAcceleration = boid.maxAcceleration + (closestBoidInfo.distance / boid.perception) * boid.maxAcceleration * 2.0;
	acceleration = clamp(acceleration, -chaseAcceleration, chaseAcceleration);
	float3 velocity = clamp(boid.velocity + acceleration, -boid.maxVelocity, boid.maxVelocity);

	BoidBufferOut[index].velocity = velocity;

	// Acceleration attenuation
	acceleration = lerp(acceleration, 0.0, 0.01);
	BoidBufferOut[index].acceleration = acceleration;

	// Apply to transform
	position += velocity * deltaTime;
	InstanceBufferOut[index].instanceModel[0].w = position.x;
	InstanceBufferOut[index].instanceModel[1].w = position.y;
	InstanceBufferOut[index].instanceModel[2].w = position.z;

	// Process Orientation
	float3 velocityDir = normalize(velocity);
	float yaw = atan2(velocityDir.y, velocityDir.x);
	float pitch = acos(velocityDir.z) - (0.5 * pi);

	float sinYaw, cosYaw, sinPitch, cosPitch;
	sincos(yaw, sinYaw, cosYaw);
	sincos(pitch, sinPitch, cosPitch);

	InstanceBufferOut[index].instanceModel[0][0] = cosYaw * cosPitch;
	InstanceBufferOut[index].instanceModel[0][1] = -sinYaw;
	InstanceBufferOut[index].instanceModel[0][2] = cosYaw * sinPitch;
	InstanceBufferOut[index].instanceModel[1][0] = sinYaw * cosPitch;
	InstanceBufferOut[index].instanceModel[1][1] = cosYaw;
	InstanceBufferOut[index].instanceModel[1][2] = sinYaw * sinPitch;
	InstanceBufferOut[index].instanceModel[2][0] = -sinPitch;
	InstanceBufferOut[index].instanceModel[2][1] = 0.0;
	InstanceBufferOut[index].instanceModel[2][2] = cosPitch;

	// Update Matrices
	InstanceBufferOut[index].model = mul(model, InstanceBufferIn[index].instanceModel);
	InstanceBufferOut[index].invTransModel = mul(
		transpose(InverseRigidTransform(InstanceBufferIn[index].instanceModel)), 
		invTransModel
	);
}