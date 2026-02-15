#include "EndfieldCommon.hlsl"
#include "../Common.hlsl"
#include "../ColorConversion.hlsl"
#include "../ShadingModels.hlsl"
#include "../VertexStructs.hlsl"

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(StandardFSInput input) {
	FSOutput output;

	half4 baseColor = albedoTex.Sample(albedoTexSampler, input.texCoord0);
	half4 property = propertyTex.Sample(propertyTexSampler, input.texCoord0);

	float3x3 tbn = {
		input.worldTangent, 
		input.worldBitangent, 
		input.worldNormal
	};
	tbn = input.isFrontFace ? tbn : -tbn;
	half3 normal = mul(LoadNormalTextureFromXY(normalTex, normalTexSampler, input.texCoord0), tbn);
	float3 view = normalize(cameraPosition.xyz - input.worldPosition);

	// Gram-Schmidt Orthogonization
	// half3 tangent = normalize(tbn[0] - dot(tbn[0], normal) * normal);
	// half3 bitangent = cross(normal, tangent);

	// Thin film interference
	half4 rampColor = 0.0;
	half4 mask = maskTex.Sample(maskTexSampler, input.texCoord0);
	if (mask.x > 0.0) {
		float rampCoord = clamp(dot(view, normal), 0.0, 1.0);
		rampColor = rampTex.Sample(rampTexSampler, float2(rampCoord, 0.5));
		baseColor.xyz = rampColor.xyz;
	}

	// Rim light
	float3 vertNormal = input.isFrontFace ? input.worldNormal : -input.worldNormal;
	float3 rimLighting = EndfieldRimLighting(vertNormal, view, property.w, property.z, rimLightWidth);

	ShadingContext shadingContext = (ShadingContext)0;
	shadingContext.baseColor = baseColor.xyz * property.z;
	shadingContext.metallic = property.x + rampColor;
	shadingContext.specular = 0.5;
	shadingContext.roughness = 1.0 - property.w;
	shadingContext.ambientOcclusion = property.z;
	// shadingContext.anisotropy = 0.8; 
	shadingContext.normal = normal; 
	// shadingContext.tangent = tangent;
	// shadingContext.bitangent = bitangent;
	shadingContext.view = view;
	// shadingContext.subsurfaceColor = sssColor * 1.0;
	// shadingContext.opacity = 0.8;

	LightingResult lightingResult = DefaultShading(shadingContext);
	float3 finalColor = lightingResult.diffuse + lightingResult.specular + lightingResult.transmission;
	finalColor += rimLighting;

	output.color.xyz = finalColor; 
	// output.color.xyz += ; 
	output.color.a = baseColor.a;

	return output;
}