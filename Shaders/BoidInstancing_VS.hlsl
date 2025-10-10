#include "Common.hlsl"
#include "CommonStructs.hlsl"

StandardVSOutput main(VSInput input) {
	StandardVSOutput output;
	InstanceBufferStruct instanceInfo = InstanceBufferIn[input.instanceID];
	float4x4 instanceTransform = {
		instanceInfo.transformR0, 
		instanceInfo.transformR1, 
		instanceInfo.transformR2, 
		instanceInfo.transformR3, 
	};
	// Not scaling for better performance.
	float4x4 invTransInstanceModel = mul(transpose(InverseRigidTransform(instanceTransform)), invTransModel);

	float4 worldPosition = mul(model, float4(input.position, 1.0));
	worldPosition = mul(instanceTransform, worldPosition);
	output.position = mul(projection, mul(view, worldPosition));
	output.worldPosition = worldPosition;

	output.worldNormal = normalize(mul(invTransInstanceModel, float4(input.normal, 0.0)).xyz);
	output.worldTangent = normalize(mul(invTransInstanceModel, float4(input.tangent, 0.0)).xyz);
	output.worldBitangent = cross(output.worldNormal, output.worldTangent);

	output.color = input.color;
	output.objectID = objectID; 
	output.texCoord0 = input.texCoord0 * float2(1.0, -1.0);
	return output;
}