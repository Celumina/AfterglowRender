#include "BxDF.hlsl"
#include "ColorConversion.hlsl"

struct ShadingContext {
	half3 baseColor;
	half metallic;
	half specular;
	half roughness;
	half ambientOcclusion; // Lower value means stronger occlusion. 0 is full occlusion.
	half anisotropy;
	half3 normal; 
	half3 tangent;
	half3 bitangent;
	half3 view; 

	// For Subsurface
	half3 subsurfaceColor; 
	half opacity;
};

struct LightingResult {
	float3 diffuse;
	float3 specular;
	float3 transmission;
}; 

struct LightContext {
	half3 color;
	float intensity;	// srcIntensity * falloff
};

// TODO: Subsurface lit context
// TODO: Light Accumulator

float3 SpecularGGX(in BxDFContext bxdfContext, float roughness, float anisotropy, float3 specularColor) {
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float2 anisotropicAlpha = AnisotropicSquaredRoughness(alpha, anisotropy);

	float3 distribution = DistributionAnisotropicGGX(
		anisotropicAlpha.x, 
		anisotropicAlpha.y, 
		bxdfContext.noh, 
		bxdfContext.xoh, 
		bxdfContext.yoh
	);

	float3 visibility = VisibilityAnisotropicSmithJoint(
		anisotropicAlpha.x, 
		anisotropicAlpha.y, 
		bxdfContext.nov, 
		bxdfContext.nol, 
		bxdfContext.xov,
		bxdfContext.xol,
		bxdfContext.yov,
		bxdfContext.yol 
	);

	float3 fresnel = FresnelSchlick(specularColor, bxdfContext.voh);

	return distribution * visibility * fresnel;
}

LightingResult EnvLighting(half3 reflectionVector, half3 baseColor, half3 specularColor, half nov, half roughness, half metallic) {
	LightingResult lighting;

	float normalizePhi = atan2(reflectionVector.y, reflectionVector.x) * (0.5 * invPi);
	float normalizeTheta = acos(reflectionVector.z) * invPi;

	// TODO: Preflitered IBL texture store into miplevels.

	// Texture info
	float textureWidth = 0; 
	float textureHeight = 0; 
	float textureNumMipMaps = 0;
	ambientTexture.GetDimensions(0, textureWidth, textureHeight, textureNumMipMaps);

	float envMipLevel = clamp(sqrt(roughness) * 10, 0, textureNumMipMaps - 1);
	// SampleLevel use absolute miplevel, instead of Sample's LOD Bias, avoids ddx/y error in large bias. 
	// SampleLevel is great, it also resolves seam problem.
	float3 envColor = ambientTexture.SampleLevel(ambientTextureSampler, float2(normalizePhi, normalizeTheta), envMipLevel).xyz;
	// There only specular, diffuse also needed.
	lighting.specular = envColor * EnvBRDFApprox(specularColor, roughness, nov);

	// TODO: Precomputed shperical harmony
	// TEMP Method
	lighting.diffuse = max(1 - metallic, 0) * baseColor * 0.08f;

	return lighting;
}

// TODO: Isotropic Default Lit

// DefaultLit: Single light overload.
LightingResult DefaultLitBxDF(in ShadingContext shadingContext, half3 lightDirection, half3 specularColor) {
	LightingResult lighting;
	lighting.transmission = 0;

	BxDFContext bxdfContext;
	InitBxDFContext(
		bxdfContext, 
		shadingContext.normal, 
		shadingContext.tangent, 
		shadingContext.bitangent, 
		shadingContext.view, 
		lightDirection
	);

	// Why UE don't do that inside the Init function, but everywhere?
	bxdfContext.nol = saturate(bxdfContext.nol);
	bxdfContext.nov = saturate(bxdfContext.nov);

	////////////////////////////////
	// Multiply falloff and falloff color here.
	lighting.diffuse = DiffuseBurley(
		shadingContext.baseColor, shadingContext.roughness, bxdfContext.nov, bxdfContext.nol, bxdfContext.voh 	
	) * bxdfContext.nol; // Beer-Lambert Law.

	lighting.specular = SpecularGGX(
		bxdfContext, shadingContext.roughness, shadingContext.anisotropy, specularColor
	) * bxdfContext.nol; // Beer-Lambert Law.
	
	////////////////////////////////

	// TODO: Energy conservation
	// References: 
	// ShadingEnergyConservation.ush
	// ShadingEnergyConservationTemplate.ush

	return lighting;
}


// TODO: Just copy to here first, after then gonna find where it from. 
LightingResult SubsurfaceBxDF(in ShadingContext shadingContext, half3 lightDirection, half3 specularColor) {
	LightingResult lighting = DefaultLitBxDF(shadingContext, lightDirection, specularColor);

	// Effect when you see through the material.
	half inScatter = pow(saturate(dot(lightDirection, -shadingContext.view)), 12) * lerp(3, 0.1f, shadingContext.opacity);

	// Warp around lighting. 
	const half warppedDiffuse = pow(saturate(dot(shadingContext.normal, lightDirection) * (1.0f / 1.5f) + (0.5f / 1.5f)), 1.5f) * (2.5f / 1.5f);
	const half normalContribution = lerp(1.0f, warppedDiffuse, shadingContext.opacity);
	const half backScatter = shadingContext.ambientOcclusion * normalContribution / (pi * 2);

	// Transmission
	// TODO: Here hardcode default thichness value as 0.15, Make it as a param.
	// Solid has a lower thicknessMeters value, conversely, gas has higher thicknessMeters value.
	// The thicknessMeters value lower, the transmittedColor saturation higher.
	// TODO: TEMP value 0.515f.
	const half3 extinctionCoefficients = TransmittanceToExtinction(shadingContext.subsurfaceColor, 0.515f);
	// 1.0f thichness meters as normlized units.
	const half3 rawTransmittedColor = ExtinctionToTransmittance(extinctionCoefficients, 1.0f);
	const half3 transmittedColor = HSVToLinearRGB(half3(LinearRGBToHSV(rawTransmittedColor).xy, LinearRGBToHSV(shadingContext.subsurfaceColor).z));

	// TODO: Second lerp weight 0 yet, here waiting for shadow thickness.
	lighting.transmission = lerp(backScatter, 1, inScatter) * lerp(transmittedColor, shadingContext.subsurfaceColor, 0);

	// lighting.transmission = rawTransmittedColor;
	return lighting;
}


#define __SHADING_TEMPLATE_BODY(BxDFFunc) \
	LightingResult lighting; \
	lighting.diffuse = 0; \
	lighting.specular = 0; \
	lighting.transmission = 0; \ 
	\
	/*GBuffer context */ \ 
	half3 specularColor  = ComputeF0(shadingContext.specular, shadingContext.baseColor, shadingContext.metallic); \
	/* TODO: For subsurface model */ \
	/*half3 diffuseColor = shadingContext.baseColor - shadingContext.baseColor * shadingContext.metallic; */ \
	\
	/*Directional Light*/ \
	LightContext directionalLightContext; \
	directionalLightContext.color = dirLightColor.xyz; \
	directionalLightContext.intensity = dirLightColor.w; \
	LightingResult directionalLighting = BxDFFunc(shadingContext, dirLightDirection.xyz, specularColor); \
	float3 directionalLightScale =  directionalLightContext.color * directionalLightContext.intensity; \
	\
	lighting.diffuse += directionalLighting.diffuse * directionalLightScale; \
	lighting.specular += directionalLighting.specular * directionalLightScale; \
	lighting.transmission += directionalLighting.transmission * directionalLightScale; \
	\
	/* TODO: Punctual Lights */ \
	\
	/* TODO: Area Lights */ \
	\
	/* Image-Based Lighting */ \
	half3 reflectionVector = reflect(-shadingContext.view, shadingContext.normal); \
	LightingResult envLighting = EnvLighting( \
		reflectionVector, \
		shadingContext.baseColor, \
		specularColor, \
		dot(shadingContext.normal, shadingContext.view), \
		shadingContext.roughness, \
		shadingContext.metallic \
	); \
	lighting.diffuse += envLighting.diffuse; \
	lighting.specular += envLighting.specular; \
	\
	return lighting; 


// DefaultLit: All lights accumulation overload.
LightingResult DefaultShading(in ShadingContext shadingContext) {
	__SHADING_TEMPLATE_BODY(DefaultLitBxDF)
	// LightingResult lighting;
	// lighting.diffuse = 0;
	// lighting.specular = 0;
	// lighting.transmission = 0;

	// // GBuffer context: 
	// half3 specularColor  = ComputeF0(shadingContext.specular, shadingContext.baseColor, shadingContext.metallic);
	// // TODO: For subsurface model
	// // half3 diffuseColor = shadingContext.baseColor - shadingContext.baseColor * shadingContext.metallic; 

	// // Directional Light
	// LightContext directionalLightContext;
	// directionalLightContext.color = dirLightColor.xyz;
	// directionalLightContext.intensity = dirLightColor.w;
	// LightingResult directionalLighting = DefaultLitBxDF(shadingContext, dirLightDirection.xyz, specularColor);
	// float3 directionalLightScale =  directionalLightContext.color * directionalLightContext.intensity;

	// lighting.diffuse += directionalLighting.diffuse * directionalLightScale;
	// lighting.specular += directionalLighting.specular * directionalLightScale;
	// lighting.transmission += directionalLighting.transmission * directionalLightScale;

	// // TODO: Punctual Lights

	// // TODO: Area Lights

	// // Image-Based Lighting
	// half3 reflectionVector = reflect(-shadingContext.view, shadingContext.normal);
	// LightingResult envLighting = EnvLighting(
	// 	reflectionVector, 
	// 	shadingContext.baseColor, 
	// 	specularColor, 
	// 	dot(shadingContext.normal, shadingContext.view), // TODO: Optimize here.
	// 	shadingContext.roughness, 
	// 	shadingContext.metallic
	// );
	// lighting.diffuse += envLighting.diffuse;
	// lighting.specular += envLighting.specular;

	// return lighting;
}

LightingResult SubsurfaceShading(in ShadingContext shadingContext) {
		__SHADING_TEMPLATE_BODY(SubsurfaceBxDF)
}