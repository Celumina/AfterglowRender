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
	// half3 normal = mul(LoadNormalTextureFromXY(normalTex, normalTexSampler, input.texCoord0), tbn);
	half4 hn = hnTex.Sample(hnTexSampler, input.texCoord0);
	half3 normal = mul(ReconstructNormal(DecodeNormal(hn.xy)), tbn);
	half3 normalH = mul(ReconstructNormal(DecodeNormal(hn.zw)), tbn);

	half4 st = stTex.Sample(stTexSampler, input.texCoord0);

	float3 view = normalize(cameraPosition.xyz - input.worldPosition);

	// Gram-Schmidt Orthogonization
	half3 tangent = normalize(tbn[0] - dot(tbn[0], normal) * normal);
	half3 bitangent = cross(property.x >= 0.5 ? normal : normalH, tangent);

	// Rim light
	float3 rimLighting = EndfieldRimLighting(input.worldNormal, view, property.w, property.z, rimLightWidth);

	// Hight light color ramp
	float2 hairRampCoord = saturate(float2(abs(dot(normal, dirLightDirection.xyz)), 0.5));
	float4 hairRamp = hairRampTex.Sample(hairRampTexSampler, hairRampCoord);

	ShadingContext shadingContext = (ShadingContext)0;
	shadingContext.baseColor = baseColor.xyz * property.z;
	shadingContext.metallic = 0.2; // property.y;
	shadingContext.specular = 2.0 * property.y;
	shadingContext.roughness = 0.8; //1.0 - property.w;
	shadingContext.ambientOcclusion = property.z;
	shadingContext.anisotropy = 0.85; 
	shadingContext.normal = normal; 
	shadingContext.tangent = tangent;
	shadingContext.bitangent = bitangent;
	shadingContext.view = view;
	// shadingContext.subsurfaceColor = sssColor * 1.0;
	// shadingContext.opacity = 0.8;

	LightingResult lightingResult = DefaultShadingAnisotropic(shadingContext);

	static const float specularSolidity = 0.1;
	const float minSpecularClamp = min(specularSolidity, 0.5);
	const float maxSpecularClamp = max(1.0 - specularSolidity, 0.501);

	float3 finalColor = 
		lightingResult.diffuse 
		+ smoothstep(minSpecularClamp, maxSpecularClamp, lightingResult.specular) * (hairRamp * property.w) * st.x
		+ lightingResult.transmission;
	finalColor += rimLighting;

	output.color.xyz = finalColor; 
	output.color.a = baseColor.a;

	// output.color.xyz = st.x; 

	return output;
}