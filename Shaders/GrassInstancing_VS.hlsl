
#include "Random.hlsl"
#include "Constants.hlsl"

struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION; // Screen Space Position
	[[vk::location(1)]] float3 worldPosition : POSITION;
	[[vk::location(2)]] float3 worldNormal : NORMAL;
	[[vk::location(5)]] float4 color : COLOR;
	[[vk::location(6)]] float2 texCoord0 : TEXCOORD0;
	[[vk::location(7)]] uint objectID : OBJECT_ID;
};

VSOutput main(VSInput input) {
	VSOutput output;
	
	InstanceBufferStruct instanceInfo = InstanceBufferIn[input.instanceID];

	float4 worldPosition = mul(instanceInfo.model, float4(input.position, 1.0));
	output.worldPosition = worldPosition.xyz;

	// disort
	float3 vertexOffset = 0.0;
	float2 distortionFrequency = float2(2.0, 1.45) * (input.color.x + 250.0) * 0.004;
	float randonSpeedFactor = (10.0 + UniformBitHash(input.instanceID)) * 0.2;
	float amplitude = 0.2;
	vertexOffset.xy = sin((time * distortionFrequency * randonSpeedFactor + worldPosition.xy * 0.01) * amplitude) * 20.0;
	// Mask
	vertexOffset *= input.color;

	worldPosition.xyz += vertexOffset;

	output.position = mul(projection, mul(view, worldPosition));
	output.worldNormal = normalize(mul(instanceInfo.invTransModel, float4(input.normal, 0.0)).xyz);
	output.color = input.color;
	output.objectID = objectID; 
	output.texCoord0 = input.texCoord0 * float2(1.0, -1.0);
	return output;
}