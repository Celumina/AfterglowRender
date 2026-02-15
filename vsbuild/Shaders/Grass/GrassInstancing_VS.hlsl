
#include "../Random.hlsl"
#include "../Constants.hlsl"

struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION; // Screen Space Position
	[[vk::location(1)]] float3 worldPosition : POSITION;
	[[vk::location(2)]] float3 worldNormal : NORMAL;
	[[vk::location(5)]] float4 color : COLOR;
	[[vk::location(7)]] uint objectID : OBJECT_ID;
};

VSOutput main(VSInput input) {
	static const float grassStiffness = 1.5;
	
	VSOutput output;
	
	InstanceBufferStruct instanceInfo = InstanceBufferIn[input.instanceID];

	float4 worldPosition = mul(instanceInfo.model, float4(input.position, 1.0));
	output.worldPosition = worldPosition.xyz;

	// Wind effect
	// @note: instanceID is unstable in indirec draw, also model height.
	float instancePosID = instanceInfo.model[0][3] + instanceInfo.model[1][3]; // + instanceInfo.model[2][3];
	float randomSpeedFactor = 2.0 * (1.0 + Hash(instancePosID, randomSeed0));// (1.0 + UniformBitHash(input.instanceID)) * 2.0;

	float2 tipFrequency = Square(1.0 - abs(sin(time * 1.0 + randomSpeedFactor - rcp(worldPosition.xy) * 0.001 * instanceInfo.wind)));
	float2 amplitude = Unorm(sin(time * 2.5 + (tipFrequency * 8.0) + worldPosition.xy * 0.5)) * 0.2 + tipFrequency * 0.8 + 0.2;
	float3 vertexOffset = 0.0;
	vertexOffset.xy = instanceInfo.wind * amplitude * pow(input.position.z, grassStiffness);
	vertexOffset.z = -length(vertexOffset.xy);
	// Mask
	vertexOffset *= Square(input.color.x);

	worldPosition.xyz += vertexOffset;

	output.position = mul(viewProjection, worldPosition);
	output.worldNormal = normalize(mul(instanceInfo.invTransModel, float4(input.normal, 0.0)).xyz);
	output.color = input.color;
	output.color.y = length(vertexOffset);
	output.objectID = objectID; 
	return output;
}