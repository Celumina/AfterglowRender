#include "../Common.hlsl"
#include "../ColorConversion.hlsl"
#include "../ShadingModels.hlsl"
#include "../VertexStructs.hlsl"

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(StandardFSInput input) {
	FSOutput output;
	static const float luminanceCorrect = 1.0;

	half4 baseColor = albedoTex.Sample(albedoTexSampler, input.texCoord0);
	half4 sssColor = sssTex.Sample(sssTexSampler, input.texCoord0);
	half4 ilm = ilmTex.Sample(ilmTexSampler, input.texCoord0);

	half3 normal = input.worldNormal;

	float3 view = normalize(cameraPosition.xyz - input.worldPosition);

	BxDFContext bxdfContext = (BxDFContext)0;
	InitBxDFContext(bxdfContext, normal, view, dirLightDirection.xyz);
	bxdfContext.nol = saturate(bxdfContext.nol);
	bxdfContext.nov = saturate(abs(bxdfContext.nov) + 1e-6);
	float diffuse = bxdfContext.nol * input.color.x;
	diffuse = smoothstep(0.49, 0.50, diffuse);

	float specular = 0.5;
	float metallic = baseColor.a;
	float roughness = 0.5;

	half3 specularColor  = ComputeF0(specular, baseColor.xyz, metallic);

	half3 reflectionVector = reflect(-view, normal);
	LightingResult envLighting = EnvLighting( 
		reflectionVector, baseColor.xyz, specularColor, bxdfContext.nov, 1.0, metallic
	);

	float3 ggx = SpecularGGX(bxdfContext, roughness, specularColor);
	ggx = smoothstep(0.04, 0.05, ggx * ilm.z);
	ggx += envLighting.specular;

	float3 finalColor = lerp(sssColor, baseColor, diffuse).xyz;
	finalColor += ggx;
	finalColor += envLighting.diffuse;

	// Internal lines
	finalColor *= ilm.w;
	// Light effect
	finalColor *= dirLightColor.xyz * dirLightColor.a;
	// Color correct
	finalColor = finalColor * luminanceCorrect * ilm.y;

	output.color.xyz = finalColor; //input.color.y;
	return output;
}