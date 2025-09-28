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

// struct VSInput {
// 	// Althrough we don't use semanticm, POSITION etc., we also should be keep them on.
// 	[[vk::location(0)]] float3 position : POSITION;
// 	[[vk::location(1)]] float3 normal : NORMAL;
// 	[[vk::location(2)]] float4 color : COLOR;
// 	[[vk::location(3)]] float2 texCoord0 : TEXCOORD0;
// };

struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION; // Screen Space Position
	[[vk::location(1)]] float3 worldPosition : POSITION;
	[[vk::location(2)]] float3 worldNormal : NORMAL;
	[[vk::location(3)]] float3 worldTangent : TANGENT;
	[[vk::location(4)]] float3 worldBitangent : BITANGENT;
	[[vk::location(5)]] float4 color : COLOR;
	[[vk::location(6)]] float2 texCoord0 : TEXCOORD0;
	[[vk::location(7)]] uint objectID : OBJECT_ID;
};

VSOutput main(VSInput input) {
	VSOutput output;
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