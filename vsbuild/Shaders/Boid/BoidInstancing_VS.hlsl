#include "../Common.hlsl"
#include "../VertexStructs.hlsl"

StandardVSOutput main(VSInput input) {
	StandardVSOutput output;
	InstanceBufferStruct instanceInfo = InstanceBufferIn[input.instanceID];

	float4 worldPosition = mul(instanceInfo.model, float4(input.position, 1.0));
	output.worldPosition = worldPosition.xyz;

	output.position = mul(viewProjection, worldPosition);

	output.worldNormal = normalize(mul(instanceInfo.invTransModel, float4(input.normal, 0.0)).xyz);
	output.worldTangent = normalize(mul(instanceInfo.invTransModel, float4(input.tangent, 0.0)).xyz);
	output.worldBitangent = cross(output.worldNormal, output.worldTangent);

	output.color = input.color;
	output.objectID = objectID; 
	output.texCoord0 = input.texCoord0;
	return output;
}