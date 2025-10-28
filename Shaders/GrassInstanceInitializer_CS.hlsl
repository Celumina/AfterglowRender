#include "Random.hlsl"
#include "Constants.hlsl"

[numthreads(256, 1, 1)]
void main(uint3 threadIDs : SV_DispatchThreadID) {
	uint index = threadIDs.x;

	// Generate instances in plane.
	float3 translation;

	translation.xy = Snorm(Hash2D(1.0 / float2(index, index), randomSeed0));
	translation.xy *= pow(UniformBitHash(-index), 0.35) * 40.0;

	// Uniform grid
	// translation.x = (index % 64.0) * 1.2 - 48;
	// translation.y = (index / 64.0) * 1.2 - 32;

	translation.z = pow(length(translation.xy) * 0.024, 1.8);
	InstanceBufferOut[index].translation = translation;

	// Rotate around z-axis
	float phi = Hash(1.0 / float(index), randomSeed1 * 2.124) * (2 * pi);
	float sinPhi;
	float cosPhi;
	sincos(phi, sinPhi, cosPhi);
	InstanceBufferOut[index].sinYaw = sinPhi;
	InstanceBufferOut[index].cosYaw = cosPhi;
}