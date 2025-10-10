// vk::binding(binding, set)
// [[vk::binding(0, 0)]]
// cbuffer MeshUniform {
// 	float4x4 model;
// 	float4x4 view;
// 	float4x4 projection;
// 	float4x4 invTransModel;
// };

// [[vk::binding(0, 3)]]
// cbuffer Balabala {
// 	float TEST;
// 	float TEST1;
// 	float TEST2;
// 	float TEST3;
// };

#include "CommonStructs.hlsl"

StandardVSOutput main(VSInput input) {
	StandardVSOutput output;
	float4 worldPosition = mul(model, float4(input.position, 1.0));
	output.position = mul(projection, mul(view, worldPosition));
	output.worldPosition = worldPosition;
	output.worldNormal = normalize(mul(invTransModel, float4(input.normal, 0.0)).xyz);
	output.worldTangent = normalize(mul(invTransModel, float4(input.tangent, 0.0)).xyz);
	output.worldBitangent = cross(output.worldNormal, output.worldTangent);
	output.color = input.color;
	output.objectID = objectID; // input.instanceID;
	output.texCoord0 = input.texCoord0 * float2(1.0, -1.0);
	return output;
}