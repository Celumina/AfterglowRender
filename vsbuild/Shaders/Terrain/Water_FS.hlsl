#include "../ShadingModels.hlsl"
#include "../Common.hlsl"
#include "TerrainCommon.hlsl"
#include "../Depth.hlsl"

struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float4 worldPosition : POSITION;
	bool isFrontFace : SV_ISFRONTFACE;
};

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(VSOutput input) {
	static const float edgeFadeDistance = 0.4;

	FSOutput output;
	// @note: A mesh-shaped artifact from MSAA resolve attechment is solved by
	// 	disabling depthWhite of the alpha object. 
	// 	It was caused due to the MSAA is designed for Opaque object, it coverage and depth min
	// 	could affect the transparency (color.w).
	float sceneDepth = SampleSceneDepth(int2(input.position.xy));
	float pixelDepth = SceneDepth(input.position.z);
	float depthFade = sceneDepth - pixelDepth;
	
	float deltaHeightToCamera = cameraPosition.z - input.worldPosition.z;
	float waterSediment = SampleTerrain(WaterSediment, WaterSedimentSampler, input.worldPosition.xy).x;
	float2 waterVelocity = SampleTerrain(WaterVelocity, WaterVelocitySampler, input.worldPosition.xy);

	float waterSpeedFactor = saturate(length(waterVelocity * max(rcp(deltaTime), 60.0)) * 0.5);

	half4 baseColor = lerp(
		half4(0.2, 0.4, 0.5, 1.0), 
		half4(0.4, 0.3, 0.2, 1.0), 
		saturate(waterSediment * waterInvSedimentCapability * 100.0)
	);

	baseColor = lerp(baseColor, half4(0.6, 0.6, 0.65, 1.0), waterSpeedFactor);

	float matallic = lerp(0.5, 0.1, waterSpeedFactor);
	float specular = lerp(0.5, 0.1, waterSpeedFactor);
	float roughness = lerp(0.01, 0.25, waterSpeedFactor);

	float3 normal = ReconstructNormal(SampleTerrain(TerrainNormal, TerrainNormalSampler, input.worldPosition.xy).zw);
	normal = lerp(normal, normalize(float3(normal.xy * 2.0, normal.z)), waterSpeedFactor);

	// TODO: foam
	// baseColor = lerp(baseColor, half4(0.9, 0.9, 0.9, 1.0), length(normal.xy));

	float3 view = normalize(cameraPosition.xyz - input.worldPosition);

	ShadingContext shadingContext = (ShadingContext)0;
	shadingContext.baseColor = baseColor.xyz;
	shadingContext.metallic = input.isFrontFace ? matallic : 0.0;
	shadingContext.specular = input.isFrontFace ? specular : 0.01;
	shadingContext.roughness = roughness;
	shadingContext.ambientOcclusion = 1.0;
	shadingContext.normal = normal; //input.worldNormal; 
	shadingContext.view = view;

	LightingResult lightingResult = DefaultShading(shadingContext);
	float4 finalColor;
	finalColor.xyz = lightingResult.diffuse + lightingResult.specular + lightingResult.transmission;

	finalColor.w = 
		input.isFrontFace 
		? clamp(pow(saturate(depthFade * 0.25), 0.2), 0.0, 1.0)
		: clamp(-deltaHeightToCamera * 0.01 + 0.5, 0.8, 1.0);

	finalColor.w -= 1.0 - clamp(depthFade, 0.0, edgeFadeDistance) * (1.0 / edgeFadeDistance);
	finalColor.w = max(finalColor.w, 0.0);

	output.color = finalColor;
	return output;
}