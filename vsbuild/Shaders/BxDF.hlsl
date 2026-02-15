#ifndef BXDF_HLSL
#define BXDF_HLSL

#include "Common.hlsl"
#include "Constants.hlsl"

// From BRDF.ush, Unreal Engine.

struct BxDFContext {
	half nov;	// dot(normal, view)
	half nol;	// dot(normal, light)
	half vol;	// dot(view, light)
	half noh;	// dot(normal, half)
	half voh;	// dot(view, halfVec)
	half xov;	// dot(tangentBasisX, view)
	half xol;	// dot(tangentBasisX, light)
	half xoh;	// dot(tangentBasisX, halfVec)
	half yov;	// dot(tangentBasisY, view)
	half yol;	// dot(tangentBasisY, light)
	half yoh;	// dot(tangentBasisY, halfVec)
};

static const float bxdfTolerance = 1.0e-10;

void InitBxDFContext(inout BxDFContext context, half3 normal, half3 view, half3 light) {
	context.nol = dot(normal, light);
	context.nov = dot(normal, view);
	context.vol = clamp(dot(view, light), -1, 1);
	float invLenH = rsqrt(2 + 2 * context.vol);
	context.noh = saturate((context.nol + context.nov) * invLenH);
	context.voh = saturate(invLenH + invLenH * context.vol);

	context.xov = 0.0f;
	context.xol = 0.0f;
	context.xoh = 0.0f;
	context.yov = 0.0f;
	context.yol = 0.0f;
	context.yoh = 0.0f;
}

void InitBxDFContext(inout BxDFContext context, half3 normal, half3 tangent, half3 bitangent, half3 view, half3 light) {
	context.nol = dot(normal, light);
	context.nov = dot(normal, view);
	context.vol = clamp(dot(view, light), -1, 1);
	float invLenH = rsqrt(2 + 2 * context.vol);
	context.noh = saturate((context.nol + context.nov) * invLenH);
	context.voh = saturate(invLenH + invLenH * context.vol);

	context.xov = dot(tangent, view);
	context.xol = dot(tangent, light);
	context.xoh = (context.xov + context.xol) * invLenH;
	context.yov = dot(bitangent, view);
	context.yol = dot(bitangent, light);
	context.yoh = (context.yov + context.yol) * invLenH;

	// Re-normalize to prevent unexcepted cusps from GGXAnisotropic.
	float3 hTangent = normalize(float3(context.xoh, context.yoh, context.noh)); 
	context.xoh = hTangent.x;
 	context.yoh = hTangent.y;
 	context.noh = hTangent.z;
}


half3 DiffuseLambert(half3 diffuseColor) {
	return diffuseColor * invPi;
}

// [Burley 2012, "Physically-Based Shading at Disney"]
half3 DiffuseBurley(float3 diffuseColor, float roughness, float nov, float nol, float voh) {
	float fd90 = 0.5 + 2 * voh * voh * roughness;
	float fdv = 1 + (fd90 - 1) * Pow5(1 - nov);
	float fdl = 1 + (fd90 - 1) * Pow5(1 - nol);
	return diffuseColor * (invPi * fdv * fdl);
}

// Distribution: 

// [Blinn 1977, "Models of light reflection for computer synthesized pictures"]
// a2 == alpha^2 == roughness^4
float DistributionBlinn(float a2, float noh) {
	float n = 2 / a2 - 2;
	return (n + 2) / (2 * pi) * pow(noh, n);
}

// [Beckmann 1963, "The scatting of electromagnetic waves from rough surfuaces"]
float DistributionBeckmann(float a2, float noh) {
	float noh2 = noh * noh;
	return exp((noh2 - 1) / (a2 * noh2)) / (pi * a2 * noh2 * noh2);
}

// [Walter el al. 2007, "Microfacet models for refraction through rough surfaces"]
float DistributionGGX(float a2, float noh) {
	float d = (noh * a2 - noh) * noh + 1;
	// @note: Here use a very small number to handle NaN error from / 0.
	return a2 / (pi * d * d + bxdfTolerance);
}

// [Burley 2012, "Physically-Based Shading at Disney"]
float DistributionAnisotropicGGX(float ax, float ay, float noh, float xoh, float yoh) {
	float a2 = ax * ay;
	float3 v = float3(ay * xoh, ax * yoh, a2 * noh);
	// @note: Here use a very small number to handle NaN error from / 0.
	float s = max(dot(v, v), bxdfTolerance);
	return invPi * a2 * Square(a2 / s);

	// This implementation is closer to original format:
	// float d = xoh * xoh / (ax * ax) + yoh * yoh / (ay * ay) + noh * noh;
	// return 1.0f / (pi * ax * ay * d * d);
}

// Geomtric Shadowing: 
// Here visibility means (G / 4 * nov * nol). They had been devided by BRDF's denominator.
// So BRDF equation can be simplified as f = distribution * visibility * fresnel.

// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
float VisibilitySchlick(float a2, float nov, float nol) {
	float k = sqrt(a2) * 0.5;
	// G1: Masking
	float schlickV = nov * (1 - k) + k;
	// G1: Shadowing
	float schlickL = nol * (1 - k) + k;
	return 0.25 / (schlickV * schlickL);
}

// [Heitz 2014, "Understanding the Masking-Shadowing Function in Mircofacet-Based BRDFs"]
float VisibilitySmith(float a2, float nov, float nol) {
	float smithV = nov + sqrt(nov * (nov - nov * a2) + a2);
	float smithL = nol + sqrt(nol * (nol - nol * a2) + a2);
	return rcp(smithV * smithL);
}

float VisibilitySmithJointApprox(float a2, float nov, float nol) {
	float a = sqrt(a2);
	float smithV = nol * (nov * (1 - a) + a);
	float smithL = nov * (nol * (1 - a) + a);
	// @note: Here use a very small number to handle rcp NaN error from 1 / 0.
	return 0.5 * rcp(smithV + smithL + bxdfTolerance);
}

float VisibilitySmithJoint(float a2, float nov, float nol) {
	float smithV = nol * sqrt(nov * (nov - nov * a2) + a2);
	float smithL = nov * sqrt(nol * (nol - nol * a2) + a2);
	// @note: Here use a very small number to handle rcp NaN error from 1 / 0.
	return 0.5 * rcp(smithV + smithL + bxdfTolerance);
}

float VisibilityAnisotropicSmithJoint(float ax, float ay, float nov, float nol, float xov, float xol, float yov, float yol) {
	float smithV = nol * length(float3(ax * xov, ay * yov, nov));
	float smithL = nov * length(float3(ax * xol, ay * yol, nol));
	// @note: Here use a very small number to handle rcp NaN error from 1 / 0.
	return 0.5 * rcp(smithV + smithL + bxdfTolerance);	
}


// Fresnel Reflection: 

// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
float3 FresnelSchlick(float3 specularColor, float voh) {
	float fc = Pow5(1 - voh);
	// Empirical model
	return saturate(50.0 * specularColor.g) * fc + (1 - fc) * specularColor;
}

float3 FresnelSchlick(float f0, float3 f90, float voh) {
	float fc = Pow5(1 - voh);
	return f90 * fc + (1 - fc) * f0;
}

float3 Fresnel(float3 specularColor, float voh) {
	float3 specularColorSqrt = sqrt(clamp(specularColor, float3(0, 0, 0), float3(0.99, 0.99, 0.99)));
	float3 n = (1 + specularColorSqrt) / (1 - specularColorSqrt);
	float3 g = sqrt(n * n + voh * voh - 1);
	return 0.5 * Square((g - voh) / (g + voh)) * (1 + Square(((g + voh) * voh - 1)/((g - voh) * voh + 1)));
}


// EnvBRDF: 

// TODO: float3 PrefilterEnvMap(roughness, reflecdir)

// Polynormial approximation
// [Lazarov 2013, "Getting More Physical in Call of Duty: Black Ops II"]
// ab: a is dielectric contribution, b is metallic onstribution.
half2 EnvBRDFApproxLazarov(half roughness, half nov) {
	const half4 c0 = {-1, -0.0275, -0.572, 0.022};
	const half4 c1 = {1, 0.0425, 1.04, -0.04};
	half4 r = roughness * c0 + c1;
	half a004 = min(r.x * r.x, exp2(-9.28 * nov)) * r.x + r.y;
	half2 ab = half2(-1.04, 1.04) * a004 + r.zw;
	return ab;
}

half3 EnvBRDFApprox(half3 specularColor, half roughness, half nov) {
	half2 ab = EnvBRDFApproxLazarov(roughness, nov);
	// Here use g maybe g is component of with largest luminance weight?
	float f90 = saturate(50.0 * specularColor.g);
	return specularColor * ab.x + f90 * ab.y;
}

half3 EnvBRDFApprox(half3 f0, half3 f90, half roughness, half nov) {
	half2 ab = EnvBRDFApproxLazarov(roughness, nov);
	return f0 * ab.x + f90 * ab.y;
}

// Miscellaneous

// [Kulla 2017, "Revisting Physically Based Shadings at Imageworks"]
// alpha == roughness^2
float2 AnisotropicSquaredRoughness(float alpha, float anisotropy) {
	float2 anisotropicRoughness;
	anisotropicRoughness.x = max(alpha * (1.0 + anisotropy), 0.001f);
	anisotropicRoughness.y = max(alpha * (1.0 - anisotropy), 0.001f);
	return anisotropicRoughness;
}

float2 AnisotropicRoughness(float roughness, float anisotropy) {
	float2 anisotropicRoughness = saturate(roughness);
	anisotropy = clamp(anisotropy, -1.0, 1.0);
	anisotropicRoughness.x = max(roughness * sqrt(1.0 + anisotropy), 0.001f);
	anisotropicRoughness.y = max(roughness * sqrt(1.0 - anisotropy), 0.001f);
	return anisotropicRoughness;
}

// Subsurface utilities
// MFP: Mean Free Path
static const float participatingMediaMinMFPMeter = 1e-12f;
static const float participatingMediaMinTransmittance = 1e-12f;

float3 TransmittanceToExtinction(in float3 transmittanceColor, in float thicknessMeters) {
	// TransmisstanceColor = exp(-extinction * thickness)
	// Extinction = -log(transitionColor) / thickness
	return -log(clamp(transmittanceColor, participatingMediaMinTransmittance, 1.0f)) / max(participatingMediaMinMFPMeter, thicknessMeters);
}

float3 ExtinctionToTransmittance(in float3 extinction, in float thicknessMeters) {
	return exp(-extinction * thicknessMeters);
}

#endif