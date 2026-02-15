#include "Common.hlsl"
#include "ShadingCommon.hlsl"

// Template for Chromatic(float3) and Achromatic(float)
template<typename EnergyType>
struct BxDFEnergyTerms {
	EnergyType w; // Overall weight to scale the lobe BxDF to ensure energy conservation.
	EnergyType e; // Directional albedo of the lobe for energy preservation and lobe picking.
};

float GetEnergyLuminance(float inEnergy) {
	return inEnergy;
}

float GetEnergyLuminance(float3 inEnergy) {
	return Luminance(inEnergy);
}

/**
* @todo: Precomputed texture method.
* @return: 
* 	.x energy conservation term for mircofacet shadowing masking conrrection.
* 	.y fresnel energy correction, for make sure total reflected energy doesn't excceed physical limits.
*/ 
float2 GGXEnergyLookup(float roughness, float nov) {
	// Approximation method:
	const float energy = 1.0 - saturate(pow(roughness, nov / roughness) * ((roughness * nov + 0.0266916) / (0.466495 + nov)));
	// Nan was happen here.
	// const float energyFresnel = Pow5(1.0 - nov) * pow(2.36651 * pow(nov, 4.7703 * roughness) + 0.0387332, roughness);
	const float energyFresnel = Pow5(1.0 - nov) * pow(2.36651 * pow(nov, max(4.7703 * roughness, 1.0e-12)) + 0.0387332, roughness);
	return float2(energy, energyFresnel);
}


/**
* @note: This method for subtrate material only.
*/
float DiffuseEnergyLookup(float roughness, float nov) {
	// Precomputed-texture method: 
	/* DiffuseEnergyTexture:
	roughness
	^
	|
	|______> nov
	*/
	return diffuseEnergyTexture.SampleLevel(diffuseEnergyTextureSampler, float2(nov, roughness), 0);
}

template<typename EnergyType>
BxDFEnergyTerms<EnergyType> ComputeFresnelEnergyTerms(float2 energy, EnergyType f0, EnergyType f90) {
	BxDFEnergyTerms<EnergyType> result;
	result.w = 1.0 + f0 * ((1.0 - energy.x) / energy.x);
	result.e = result.w * (energy.x * f0 + energy.y * (f90 - f0));
	return result;
}

template<typename EnergyType>
BxDFEnergyTerms<EnergyType> ComputeGGXSpecularEnergyTerms(float roughness, float nov, float3 f0, float3 f90) {
	return ComputeFresnelEnergyTerms(GGXEnergyLookup(roughness, nov), f0, f90);
}

template<typename EnergyType>
BxDFEnergyTerms<EnergyType> ComputeGGXSpecularEnergyTerms(float roughness, float nov, float3 f0) {
	const float f90 = F0RGBToMircoOcclusion(f0);
	return ComputeGGXSpecularEnergyTerms<EnergyType>(roughness, nov, f0, f90);
}

template<typename EnergyType>
BxDFEnergyTerms<EnergyType> ComputeDiffuseEnergyTerms(float roughness, float nov) {
	BxDFEnergyTerms<EnergyType> result;
	result.e = DiffuseEnergyLookup(roughness, nov);
	result.w = 1.0;
	return result;
}

// Manages energy transfer between layers(specular -> diffuse). Ensure energy absorbed by one laer doesn't double-count energy reflected by another.
// Return the energy absorbed by upper layer (e.g., for the specular layout attenuation onto diffuse)
template<typename EnergyType>
EnergyType ComputeEnergyPreservation(BxDFEnergyTerms<EnergyType> energyTerms) {
	return 1.0 - GetEnergyLuminance(energyTerms.e);
}

// Corrects energy loss within the BxDF model itself.
// Returns the energy conservation weight factor for account energy loss in BSDF model. (i.e. Multiple scatterin)
template<typename EnergyType>
EnergyType ComputeEnergyConservation(BxDFEnergyTerms<EnergyType> energyTerms) {
	return energyTerms.w;
}