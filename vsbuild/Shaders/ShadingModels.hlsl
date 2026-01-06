#ifndef SHADING_MODELS_HLSL
#define SHADING_MODELS_HLSL


#include "BxDF.hlsl"
#include "ShadingEnergyConservation.hlsl"
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

using EnergyTerm = float3;

// TODO: Light Accumulator

// Anisotropy version
float3 SpecularGGX(in BxDFContext bxdfContext, float roughness, float anisotropy, float3 specularColor) {
	float alpha = roughness * roughness;
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

// Isotropy version
float3 SpecularGGX(in BxDFContext bxdfContext, float roughness, float3 specularColor) {
	float alpha2 = Pow4(roughness);
	
	// TODO: Area light support
	// float Energy = EnergyNormalization( alpha2, bxdfContext.voh, areaLight);

	float distribution = DistributionGGX(alpha2, bxdfContext.noh);
	float visibility = VisibilitySmithJointApprox(alpha2, bxdfContext.nov, bxdfContext.nol);
	float3 fresnel = FresnelSchlick(specularColor, bxdfContext.voh);

	return distribution * visibility * fresnel;
}

LightingResult EnvLighting(half3 reflectionVector, half3 baseColor, half3 specularColor, half nov, half roughness, half metallic) {
	LightingResult lighting;

	float normalizePhi = (atan2(reflectionVector.y, reflectionVector.x)) * (0.5 * invPi) + 0.5;
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

#define __DEFALUT_LIT_BXDF_BODY(initBxDFExpression,specularGGXExpression) \
	LightingResult lighting; \
	lighting.transmission = 0; \
	\
	BxDFContext bxdfContext; \
	initBxDFExpression; \
	\
	/* Why UE don't do that inside the Init function, but everywhere? */ \
	bxdfContext.nol = saturate(bxdfContext.nol); \
	bxdfContext.nov = saturate(bxdfContext.nov); \
	\
	/* TODO: Multiply falloff and falloff color here. */ \
	/* TODO: Area light support. */ \
	lighting.diffuse = DiffuseBurley( \
		shadingContext.baseColor, shadingContext.roughness, bxdfContext.nov, bxdfContext.nol, bxdfContext.voh \
	) * bxdfContext.nol; /* Beer-Lambert Law. */ \
	\
	lighting.specular = specularGGXExpression * bxdfContext.nol; /* Beer-Lambert Law. */ \
	\
	BxDFEnergyTerms<EnergyTerm> energyTerms = ComputeGGXSpecularEnergyTerms<EnergyTerm>(shadingContext.roughness, bxdfContext.nov, specularColor); \
	/* DiffusePreservation: Attenuation of the specular layer onto the diffuse.*/ \
	lighting.diffuse *= ComputeEnergyPreservation(energyTerms); \
	/* SpecularCovervation: Add specular mircofacet multiple scattering term. */ \
	lighting.specular *= ComputeEnergyConservation(energyTerms); \
	\
	return lighting;


LightingResult DefaultLitBxDF(in ShadingContext shadingContext, half3 lightDirection, half3 specularColor) {
	__DEFALUT_LIT_BXDF_BODY(
		InitBxDFContext(bxdfContext, shadingContext.normal, shadingContext.view, lightDirection), 
		SpecularGGX(bxdfContext, shadingContext.roughness, specularColor)
	)
}

// DefaultLit: Single light overload.
LightingResult DefaultLitBxDFAnisotropic(in ShadingContext shadingContext, half3 lightDirection, half3 specularColor) {
	__DEFALUT_LIT_BXDF_BODY(
		InitBxDFContext(
			bxdfContext, 
			shadingContext.normal, 
			shadingContext.tangent, 
			shadingContext.bitangent, 
			shadingContext.view, 
			lightDirection
		), 
		SpecularGGX(bxdfContext, shadingContext.roughness, shadingContext.anisotropy, specularColor)
	)	
}


#define __SUBSURFACE_BXDF_BODY(defaultBxDFFunc) \
	LightingResult lighting = defaultBxDFFunc(shadingContext, lightDirection, specularColor); \
	\
	/* Effect when you see through the material. */ \
	half inScatter = pow(saturate(dot(lightDirection, -shadingContext.view)), 12) * lerp(3, 0.1f, shadingContext.opacity); \
	\
	/* Warp around lighting. */ \
	const half warppedDiffuse = pow(saturate(dot(shadingContext.normal, lightDirection) * (1.0f / 1.5f) + (0.5f / 1.5f)), 1.5f) * (2.5f / 1.5f); \
	const half normalContribution = lerp(1.0f, warppedDiffuse, shadingContext.opacity); \
	const half backScatter = shadingContext.ambientOcclusion * normalContribution / (pi * 2); \
	\
	/* Transmission */ \
	/* TODO: Here hardcode default thichness value as 0.15, Make it as a param. */ \
	/* Solid has a lower thicknessMeters value, conversely, gas has higher thicknessMeters value. */ \
	/* The thicknessMeters value lower, the transmittedColor saturation higher. */ \
	/* TODO: TEMP value 0.515f. */ \
	const half3 extinctionCoefficients = TransmittanceToExtinction(shadingContext.subsurfaceColor, 0.515f); \
	/* 1.0f thichness meters as normlized units. */ \
	const half3 rawTransmittedColor = ExtinctionToTransmittance(extinctionCoefficients, 1.0f); \
	const half3 transmittedColor = HSVToLinearRGB(half3(LinearRGBToHSV(rawTransmittedColor).xy, LinearRGBToHSV(shadingContext.subsurfaceColor).z)); \
	\
	/* TODO: Second lerp weight 0 yet, here waiting for shadow thickness. */ \
	lighting.transmission = lerp(backScatter, 1, inScatter) * lerp(transmittedColor, shadingContext.subsurfaceColor, 0); \
	\
	/* lighting.transmission = rawTransmittedColor; */ \
	return lighting;


LightingResult SubsurfaceBxDF(in ShadingContext shadingContext, half3 lightDirection, half3 specularColor) {
	__SUBSURFACE_BXDF_BODY(DefaultLitBxDF)
}

LightingResult SubsurfaceBxDFAnisotropic(in ShadingContext shadingContext, half3 lightDirection, half3 specularColor) {
	__SUBSURFACE_BXDF_BODY(DefaultLitBxDFAnisotropic)
}


#define __SHADING_BODY(bxdfFunc) \
	LightingResult lighting = (LightingResult)0; \
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
	LightingResult directionalLighting = bxdfFunc(shadingContext, dirLightDirection.xyz, specularColor); \
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
	__SHADING_BODY(DefaultLitBxDF)
}

LightingResult DefaultShadingAnisotropic(in ShadingContext shadingContext) {
	__SHADING_BODY(DefaultLitBxDFAnisotropic)
}

LightingResult SubsurfaceShading(in ShadingContext shadingContext) {
	__SHADING_BODY(SubsurfaceBxDF)
}

LightingResult SubsurfaceShadingAnisotropic(in ShadingContext shadingContext) {
	__SHADING_BODY(SubsurfaceBxDFAnisotropic)
}


#endif