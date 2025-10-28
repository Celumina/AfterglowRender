#include "Common.hlsl"

// TODO: Try to calculate the ClipSpace position and give a discard mark.
[numthreads(256, 1, 1)]
void main(uint3 threadIDs : SV_DispatchThreadID) {
	uint index = threadIDs.x;

	float4x4 instanceModel = float4x4(
		InstanceBufferIn[index].cosYaw, -InstanceBufferIn[index].sinYaw, 0.0, InstanceBufferIn[index].translation.x, 
		InstanceBufferIn[index].sinYaw, InstanceBufferIn[index].cosYaw, 0.0, InstanceBufferIn[index].translation.y, 
		0.0, 0.0, 1.0, InstanceBufferIn[index].translation.z, 
		0.0, 0.0, 0.0, 1.0
	);

	// Provide invTransModel and combined mvp
	InstanceBufferOut[index].model = mul(model, instanceModel);
	InstanceBufferOut[index].invTransModel = mul(
		transpose(InverseRigidTransform(instanceModel)), 
		invTransModel
	);
}