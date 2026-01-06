// vk::binding(binding, set)
// [[vk::binding(1, 0)]]
// cbuffer SceneUniform {
// 	float4 dirLightDirection;
// 	float4 dirLightColor;
// 	float4 cameraPosition;
// 	float4 cameraVector;
// 	float2 screenResolution;
// 	float time;
// };

// [[vk::combinedImageSampler]] [[vk::binding(1, 2)]] 
// Texture2D<float4> albedoTex;

// [[vk::combinedImageSampler]] [[vk::binding(1, 2)]] 
// SamplerState albedoTexSampler;

#include "Common.hlsl"
#include "ColorConversion.hlsl"
#include "ShadingModels.hlsl"
#include "VertexStructs.hlsl"

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
	// [[vk::location(1)]] uint stencil : SV_STENCILREF;
};

FSOutput main(StandardFSInput input) {
	FSOutput output;

	float3x3 tbn = {
		input.worldTangent, 
		input.worldBitangent, 
		input.worldNormal
	};
	tbn = input.isFrontFace ? tbn : -tbn;

	half4 baseColor = albedoTex.Sample(albedoTexSampler, input.texCoord0);
	// tbn defines as row-major so put matrix in the rightside.
	half3 normal = mul(DecodeNormal(normalTex.Sample(normalTexSampler, input.texCoord0 * 4.0).xyz), tbn);	
	// normal = input.worldNormal;
	
	// Gram-Schmidt Orthogonization
	half3 tangent = normalize(tbn[0] - dot(tbn[0], normal) * normal);
	half3 bitangent = cross(normal, tangent);

	float3 view = normalize(cameraPosition.xyz - input.worldPosition);

	// if (baseColor.x < 0.01) {
	// 	clip(-1);
	// }

	ShadingContext shadingContext = (ShadingContext)0;
	shadingContext.baseColor = baseColor.xyz;
	shadingContext.metallic = 0.1;
	shadingContext.specular = 0.5;
	shadingContext.roughness = 0.5;
	shadingContext.ambientOcclusion = 1.0;
	shadingContext.anisotropy = 0.5; 
	shadingContext.normal = normal; //input.worldNormal; 
	shadingContext.tangent = tangent;
	shadingContext.bitangent = bitangent;
	shadingContext.view = view;
	shadingContext.subsurfaceColor = baseColor * 1.0;
	shadingContext.opacity = 0.99;

	LightingResult lightingResult = SubsurfaceShadingAnisotropic(shadingContext);
	float3 finalColor = lightingResult.diffuse + lightingResult.specular + lightingResult.transmission;
	
	// finalColor = lightingResult.specular;

	output.color.xyz = finalColor;
	// output.color.xyz = input.color;
	// output.color.xyz = baseColor; //HSVToLinearRGB(LinearRGBToHSV(baseColor));


	return output;
}