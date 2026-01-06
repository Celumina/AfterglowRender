#include "GrassCommon.hlsl"
#include "../Random.hlsl"
#include "../Constants.hlsl"
#include "../ColorConversion.hlsl"
#include "../Terrain/TerrainCommon.hlsl"


struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION; // Screen Space Position
	[[vk::location(1)]] float3 worldPosition : POSITION;
	[[vk::location(2)]] float3 worldNormal : NORMAL;
	[[vk::location(5)]] float4 color : COLOR;
	[[vk::location(7)]] uint objectID : OBJECT_ID;
	bool isFrontFace : SV_ISFRONTFACE;
};

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
	// [[vk::location(1)]] uint stencil : SV_STENCILREF;
};

FSOutput main(VSOutput input) {	
	// clip(-1);
	FSOutput output;
	
	half3 baseColor = lerp(float3(0.08, 0.1, 0.04), float3(0.2, 0.06, 0.02), saturate(PerlinNoise(input.worldPosition.xy, float2(0.2, 0.2), randomSeed0) - 0.2));
	baseColor = mad(baseColor, (input.color.y * 2.0), baseColor);
	baseColor = Desaturation(baseColor, -(input.color.y * 0.5));
	// baseColor = -Square(input.color.y * 0.01);
	// Ambient occlusion (approximation)
	half3 normal = input.isFrontFace ? input.worldNormal : -input.worldNormal;

	output.color.xyz = lerp(baseColor.xyz, baseColor.xyz * max(dot(normal, dirLightDirection.xyz), 0.0), 0.5);
	output.color.xyz *= dirLightColor.xyz * dirLightColor.w;

	// Blend terrain
	half4 terrainSurface = SampleTerrain(TerrainSurface, TerrainSurfaceSampler, input.worldPosition.xy);
	output.color.xyz = lerp(output.color.xyz, terrainSurface.xyz, 0.75/*input.color.x*/);

	// Fake occlusion
	float occlusionFactor = min(input.position.z * grassVisibleDistance, 0.8);
	output.color.xyz = lerp(output.color.xyz, float3(0.03, 0.03, 0.02), (1.0 - Pow3(input.color.x)) * occlusionFactor);
	return output;	 
}