#include "../Random.hlsl"
#include "../Constants.hlsl"

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	uint index = threadID.x;

	// Generate instances in plane.
	float3 translation = 0.0;

	// UniformGrid + RandomOffset
	// static const uint grassGridSideElements = 256;
	// static const float grassInvGridSideElements = 1.0 / grassGridSideElements; 
	// static const float grassGridInterval = 0.25;
	// translation.x = ((index % grassGridSideElements) - (grassGridSideElements * 0.5)) * grassGridInterval;
	// translation.y = ((index / grassGridSideElements) - (grassGridSideElements * 0.5)) * grassGridInterval;
	// translation.xy += Snorm(Hash2D(float2(index, index) * grassInvGridSideElements, randomSeed0));

	// Random Distribution
	translation.xy = Snorm(Hash2D(float2(index, index) * 0.001, randomSeed0 - index)) * 32.0;

	InstanceBufferOut[index].model._m03_m13_m23 = translation;

	// Rotate around z-axis
	float phi = Hash(1.0 / float(index), randomSeed1 * 2.124) * (2 * pi);
	float sinPhi;
	float cosPhi;
	sincos(phi, sinPhi, cosPhi);
	InstanceBufferOut[index].sinYaw = sinPhi;
	InstanceBufferOut[index].cosYaw = cosPhi;

	// Random scaling epsilon=0.2
	InstanceBufferOut[index].scaling = Snorm(Hash(1.0 / index, randomSeed1)) * 0.2 + 1.0;

	// Initialization direction of Wind
	InstanceBufferOut[index].wind = float2(0.0, 0.0);
}