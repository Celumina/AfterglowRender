#include "../ShadingModels.hlsl"
#include "../Common.hlsl"
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


	half4 baseColor = SampleTerrain(TerrainSurface, TerrainSurfaceSampler, input.worldPosition.xy);// half4(1.0, 1.0, 1.0, 1.0);
	float3 normal = ReconstructNormal(SampleTerrain(TerrainNormal, TerrainNormalSampler, input.worldPosition.xy).xy);
	float3 detailNormal = ReconstructNormal(FractalNormal.SampleLevel(FractalNormalSampler, input.worldPosition.xy * 0.2, 0).xy);
	normal = BlendAngleCorrectedNormals(normal, detailNormal);

	// half3 normal = ReconstructNormal(LoadTerrain(TerrainNormal, input.worldPosition.xy).xy);
	float3 view = normalize(cameraPosition.xyz - input.worldPosition);

	ShadingContext shadingContext = (ShadingContext)0;
	shadingContext.baseColor = baseColor.xyz;
	shadingContext.metallic = 0.0;
	shadingContext.specular = 0.5;
	shadingContext.roughness = 0.9;
	shadingContext.ambientOcclusion = 1.0;
	shadingContext.normal = normal; //input.worldNormal; 
	shadingContext.view = view;

	LightingResult lightingResult = DefaultShading(shadingContext);
	float3 finalColor = lightingResult.diffuse + lightingResult.specular + lightingResult.transmission;

	// finalColor = normal;
	// finalColor = FractalNoise.SampleLevel(FractalNoiseSampler, input.worldPosition.xy * 0.2, 0).xxx;

	output.color.xyz = finalColor;
	return output;
}