#include "GrassCommon.hlsl"
#include "../Random.hlsl"
#include "../Constants.hlsl"
#include "../ColorConversion.hlsl"
#include "../Terrain/TerrainCommon.hlsl"
#include "../Meteorograph/MeteorographCommon.hlsl"
#include "../ShadingModels.hlsl"

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
	
	// half3 normal = normalize(cross(ddy(input.worldPosition.xyz), ddx(input.worldPosition.xyz)));
	
	half4 fractalNormal = FractalNormal.Sample(FractalNormalSampler, float2(input.worldPosition.x + input.worldPosition.y, input.color.x) * 4.0);

	half3 normal = input.isFrontFace ? input.worldNormal : -input.worldNormal;
	// half3 normal = input.worldNormal;
	half4 terrainNormal = SampleTerrain(TerrainNormal, TerrainNormalSampler, input.worldPosition.xy);
	normal = normalize(lerp(normal, ReconstructNormal(terrainNormal.xy), 0.75));
	normal = BlendAngleCorrectedNormals(normal, ReconstructNormal(fractalNormal.xy * 2.0));

	// Half-Lambert Lighting
	// output.color.xyz = baseColor.xyz * max(Unorm(dot(normal, dirLightDirection.xyz)), 0.0);

	// Blend terrain
	float2 terrainTexCoord = input.worldPosition.xy * terrainTexCoordScaling;
	half4 texColor = grassAlbedoTex.SampleLevel(grassAlbedoTexSampler, terrainTexCoord, 3) * 2.0;
	half4 terrainSurface = SampleTerrain(TerrainSurface, TerrainSurfaceSampler, input.worldPosition.xy);
	float4 meteorograph = SampleMeteorograph(Meteorograph, MeteorographSampler, input.worldPosition.xy);
	texColor.xyz = VariantTerrainSurface(texColor.xyz, terrainSurface, meteorograph.w);
	baseColor.xyz = lerp(baseColor.xyz, texColor.xyz, 0.85) * 0.75;
	
	// Lighting
	ShadingContext shadingContext = (ShadingContext)0;
	shadingContext.baseColor = baseColor.xyz;
	shadingContext.metallic = 0.0;
	shadingContext.specular = 0.1;
	shadingContext.roughness = 0.75;
	shadingContext.ambientOcclusion = 1.0; 
	shadingContext.normal = normal;  
	shadingContext.view = normalize(cameraPosition.xyz - input.worldPosition.xyz);

	LightingResult lightingResult = DefaultShading(shadingContext);
	output.color.xyz = max(lightingResult.diffuse, baseColor.xyz * 0.5) + lightingResult.specular + lightingResult.transmission;

	// Fake occlusion
	float occlusionFactor = min(input.position.z * grassVisibleDistance, 0.85);
	output.color.xyz = lerp(output.color.xyz, baseColor.xyz * 0.2, (1.0 - Pow3(input.color.x)) * occlusionFactor);

	output.color.xyz *= dirLightColor.xyz * dirLightColor.w;
	// output.color.xyz = normal;

	return output;	 
}