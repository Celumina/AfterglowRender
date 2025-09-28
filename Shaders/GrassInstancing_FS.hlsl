#include "Random.hlsl"


struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION; // Screen Space Position
	[[vk::location(1)]] float3 worldPosition : POSITION;
	[[vk::location(2)]] float3 worldNormal : NORMAL;
	[[vk::location(5)]] float4 color : COLOR;
	[[vk::location(6)]] float2 texCoord0 : TEXCOORD0;
	[[vk::location(7)]] uint objectID : OBJECT_ID;
};

struct PSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
	// [[vk::location(1)]] uint stencil : SV_STENCILREF;
};

PSOutput main(VSOutput input) {
	PSOutput output;
	 
	half3 baseColor = lerp(float3(0.2, 0.3, 0.1), float3(0.8, 0.3, 0.1), PerlinNoise(input.worldPosition.xy, float2(0.002, 0.002), randomSeed0) - 0.4);
	baseColor = lerp(float3(0.05, 0.03, 0.02), baseColor, Pow3(input.color.x));

	output.color.xyz = lerp(baseColor.xyz, baseColor.xyz * max(dot(input.worldNormal, dirLightDirection.xyz), 0.0), 0.5);
	// output.color.xyz = PerlinNoise(input.worldPosition.xy, float2(0.001, 0.001), randomSeed0) - 0.5;
	return output;
}