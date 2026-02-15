#include "GrassCommon.hlsl"
#include "../Random.hlsl"
#include "../Constants.hlsl"
#include "../ColorConversion.hlsl"
#include "../Terrain/TerrainCommon.hlsl"
#include "../Meteorograph/MeteorographCommon.hlsl"


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
	// Half-Lambert Lighting
	output.color.xyz = baseColor.xyz * max(Unorm(dot(normal, dirLightDirection.xyz)), 0.0);

	// Blend terrain
	float2 terrainTexCoord = input.worldPosition.xy * terrainTexCoordScaling;
	half4 texColor = grassAlbedoTex.SampleLevel(grassAlbedoTexSampler, terrainTexCoord, 3) * 4.0;
	half4 terrainSurface = SampleTerrain(TerrainSurface, TerrainSurfaceSampler, input.worldPosition.xy);
	float4 meteorograph = SampleMeteorograph(Meteorograph, MeteorographSampler, input.worldPosition.xy);
	texColor.xyz = VariantTerrainSurface(texColor.xyz, terrainSurface, meteorograph.w);
	// output.color.xyz = lerp(output.color.xyz, terrainSurface.xyz * 0.5, 0.65/*input.color.x*/) * 0.75;
	output.color.xyz = lerp(output.color.xyz, texColor.xyz * 0.5, 0.65) * 0.75;
	
	// Fake occlusion
	float occlusionFactor = min(input.position.z * grassVisibleDistance, 0.8);
	output.color.xyz = lerp(output.color.xyz, output.color.xyz * 0.4, (1.0 - Pow3(input.color.x)) * occlusionFactor);

	output.color.xyz *= dirLightColor.xyz * dirLightColor.w;
	// output.color.xyz = input.color.w;

	return output;	 
}