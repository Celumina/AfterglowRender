#include "Random.hlsl"
#include "Constants.hlsl"

[numthreads(256, 1, 1)]
void main(uint3 threadIDs : SV_DispatchThreadID) {
	uint index = threadIDs.x;

	// Generate instances in plane.
	float3 translation;
	translation.xy = Snorm(Hash2D(float2(1.0 / index, 1.0 / index), randomSeed0));
	translation.xy *= pow(UniformBitHash(-index), 0.45) * 8000.0;
	translation.z = pow(distance(translation.xy, 0.0) * 0.0032, 1.85);
	// Transfrom matrix store by row, but it is a Column-major matrix in actual.
	InstanceBufferOut[index].transformR0.w = translation.x;
	InstanceBufferOut[index].transformR1.w = translation.y;
	InstanceBufferOut[index].transformR2.w = translation.z;
	InstanceBufferOut[index].transformR3.w = 1.0;

	// Rotate around z-axis
	float theta = Hash(float(index), randomSeed0) * (2 * pi);
	float sinTheta;
	float cosTheta;
	sincos(theta, sinTheta, cosTheta);
	InstanceBufferOut[index].transformR0.x = cosTheta;
	InstanceBufferOut[index].transformR0.y = -sinTheta;
	InstanceBufferOut[index].transformR1.x = sinTheta;
	InstanceBufferOut[index].transformR1.y = cosTheta;
	InstanceBufferOut[index].transformR2.z = 1.0;
}