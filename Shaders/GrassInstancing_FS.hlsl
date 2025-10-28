#include "Random.hlsl"
#include "Constants.hlsl"


struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION; // Screen Space Position
	[[vk::location(1)]] float3 worldPosition : POSITION;
	[[vk::location(2)]] float3 worldNormal : NORMAL;
	[[vk::location(5)]] float4 color : COLOR;
	[[vk::location(6)]] float2 texCoord0 : TEXCOORD0;
	[[vk::location(7)]] uint objectID : OBJECT_ID;
	bool isFrontFace : SV_ISFRONTFACE;
};

struct PSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
	// [[vk::location(1)]] uint stencil : SV_STENCILREF;
};

PSOutput main(VSOutput input) {	
	// clip(-1);
	PSOutput output;
	
	half3 baseColor = lerp(float3(0.2, 0.3, 0.1), float3(0.6, 0.15, 0.05), saturate(PerlinNoise(input.worldPosition.xy, float2(0.002, 0.002), randomSeed0) - 0.2));
	// Ambient occlusion (approximation)
	half3 normal = input.isFrontFace ? input.worldNormal : -input.worldNormal;

	output.color.xyz = lerp(baseColor.xyz, baseColor.xyz * max(dot(normal, dirLightDirection.xyz), 0.0), 0.5);
	output.color.xyz *= dirLightColor.xyz * dirLightColor.w;
	output.color.xyz = lerp(float3(0.05, 0.03, 0.02), output.color.xyz, Pow3(input.color.x));
	return output;	 
}