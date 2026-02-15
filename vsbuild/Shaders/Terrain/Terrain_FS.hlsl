#include "../ShadingModels.hlsl"
#include "../Common.hlsl"
#include "../Random.hlsl"
#include "../Meteorograph/MeteorographCommon.hlsl"
#include "TerrainCommon.hlsl"

struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float4 worldPosition : POSITION;
};

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(VSOutput input) {
	FSOutput output;

	// Dither coord offset to reduce aliasing.
	// @deprecated: Textures were solved it.
	// float2 coordOffset = Snorm(Hash2D(ceil(input.worldPosition.xy * 128.0) * 0.001, randomSeed0)) * 0.25;
	half4 terrainSurface = SampleTerrain(TerrainSurface, TerrainSurfaceSampler, input.worldPosition.xy/* + coordOffset*/);

	float3 normal = ReconstructNormal(SampleTerrain(TerrainNormal, TerrainNormalSampler, input.worldPosition.xy/* + coordOffset*/).xy);
	// float3 detailNormal = ReconstructNormal(FractalNormal.SampleLevel(FractalNormalSampler, input.worldPosition.xy * 0.2, 0).xy);
	// normal = BlendAngleCorrectedNormals(normal, detailNormal);

	float3x3 tbn = {
		cross(normal, float3(1.0, 0.0, 0.0)), 
		float3(1.0, 0.0, 0.0), 
		normal
	};

	half4 texColor = 0.0;
	half3 texNormal = half3(0.0, 0.0, 1.0);
	half4 texProperty = 0.0;

	float2 terrainTexCoord = input.worldPosition.xy * terrainTexCoordScaling;

	// TODO: Here use r+g+b+a < 1.0 as condition
	texColor = grassAlbedoTex.Sample(grassAlbedoTexSampler, terrainTexCoord) * 4.0;
	texNormal = LoadNormalTexture(grassNormalTex, grassNormalTexSampler, terrainTexCoord);
	texProperty = grassPropertyTex.Sample(grassPropertyTexSampler, terrainTexCoord);

	float4 meteorograph = SampleMeteorograph(Meteorograph, MeteorographSampler, input.worldPosition.xy);
	texColor.xyz = VariantTerrainSurface(texColor.xyz, terrainSurface, meteorograph.w);
	terrainSurface.r = lerp(terrainSurface.r, min(terrainSurface.r, 0.2), saturate(-meteorograph.w * 0.5));

	if (terrainSurface.r != 0.0) {
		half4 slopeTexColor = slopeAlbedoTex.Sample(slopeAlbedoTexSampler, terrainTexCoord);
		half3 slopeTexNormal = LoadNormalTexture(slopeNormalTex, slopeNormalTexSampler, terrainTexCoord);
		texColor = lerp(texColor, float4(Desaturation(slopeTexColor.xyz, 0.5), slopeTexColor.w), terrainSurface.r);
		texNormal = BlendAngleCorrectedNormals(texNormal, slopeTexNormal);	
		texProperty = lerp(texProperty, slopePropertyTex.Sample(slopePropertyTexSampler, terrainTexCoord), terrainSurface.r);
	}

	normal = BlendAngleCorrectedNormals(normal, mul(tbn, texNormal));	

	// half3 normal = ReconstructNormal(LoadTerrain(TerrainNormal, input.worldPosition.xy).xy);
	float3 view = normalize(cameraPosition.xyz - input.worldPosition);

	ShadingContext shadingContext = (ShadingContext)0;
	shadingContext.baseColor = texColor.rgb;
	shadingContext.metallic = 0.0;
	shadingContext.specular = texProperty.z;
	shadingContext.roughness = min(texProperty.y, terrainSurface.a);
	shadingContext.ambientOcclusion = texProperty.x;
	shadingContext.normal = normal; //input.worldNormal; 
	shadingContext.view = view;

	LightingResult lightingResult = DefaultShading(shadingContext);
	float3 finalColor = lightingResult.diffuse + lightingResult.specular + lightingResult.transmission;

	output.color.xyz = finalColor;

	// output.color.xyz = texProperty.w;

	return output;
}