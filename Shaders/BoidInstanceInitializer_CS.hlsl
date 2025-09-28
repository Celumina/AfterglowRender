#include "Random.hlsl"

[numthreads(256, 1, 1)]
void main(uint3 threadIDs : SV_DispatchThreadID) {
	uint index = threadIDs.x;

	// Initialize instancing buffer
	// Translation: Spherical shape
	// Sphere center higher than the ground.
	static const float3 center = {0, 0, 200};
	float3 direction = Snorm(Hash3D(float3(index, index, index), randomSeed0));
	float radius = Hash(float(index), randomSeed0) * 200.0;
	float3 translation = direction * radius;
	// Transfrom matrix store by row, but it is a Column-major matrix in actual.
	InstanceBufferOut[index].transformR0.w = translation.x;
	InstanceBufferOut[index].transformR1.w = translation.y;
	InstanceBufferOut[index].transformR2.w = translation.z;
	InstanceBufferOut[index].transformR3.w = 1.0;

	// Rotation: Initialize by random direction
	// From quaternion to rotate matrix
	float4 quaternion = normalize(Snorm(float4(
		Hash3D(direction, randomSeed0), 
		Hash(radius, randomSeed0)
	)));
	// Quaternion to matrix
	InstanceBufferOut[index].transformR0.x = 1.0 - 2.0 * (Square(quaternion.y) + Square(quaternion.z));
	InstanceBufferOut[index].transformR0.y = 2.0 * (quaternion.x * quaternion.y - quaternion.z * quaternion.w);
	InstanceBufferOut[index].transformR0.z = 2.0 * (quaternion.x * quaternion.z + quaternion.y * quaternion.w);
	InstanceBufferOut[index].transformR1.x = 2.0 * (quaternion.x * quaternion.y + quaternion.z * quaternion.w);
	InstanceBufferOut[index].transformR1.y = 1.0 - 2.0 * (Square(quaternion.x) + Square(quaternion.z));
	InstanceBufferOut[index].transformR1.z = 2.0 * (quaternion.y * quaternion.z - quaternion.x * quaternion.w);
	InstanceBufferOut[index].transformR2.x = 2.0 * (quaternion.x * quaternion.z - quaternion.y * quaternion.w);
	InstanceBufferOut[index].transformR2.y = 2.0 * (quaternion.y * quaternion.z + quaternion.x * quaternion.w);
	InstanceBufferOut[index].transformR2.z = 1.0 - 2.0 * (Square(quaternion.x) + Square(quaternion.y));

	// @deprecated: Scaling: Variant instance size
	// @note: Never do that, decomposite matrix is complicated.
	// 	If requires, store scaling values as another parameter.
}