#include "../VertexStructs.hlsl"
#include "../Random.hlsl"
#include "../ShadingModels.hlsl"

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(StandardInstancingFSInput input) {
	static const float furThickness = 1.5;

	FSOutput output;	
	float2 furCoord = (input.texCoord0 - float2(0.0, input.instanceID * 0.0005)) * float2(16.0, 16.0);
	float noise = FractalWorleyNoise.Sample(FractalWorleyNoiseSampler, furCoord, 0, 0.0);
	// float noise = WorleyNoise(input.texCoord0 + float2(0.0, input.instanceID * 0.0002), float2(1000.0, 1000.0), randomSeed0);	
	// @note: Here clip would disable early-z
	// TODO: Aquire total instance count
	if (max(furThickness - noise, 0.0) < (input.instanceID + 0.5) * rcp(instanceCount)) {
		clip(-1);
	}

	float3x3 tbn = {
		input.worldTangent, 
		input.worldBitangent, 
		input.worldNormal
	};

	half4 albedo = albedoTex.Sample(albedoTexSampler, input.texCoord0);
	// tbn defines as row-major so put matrix in the rightside.
	half3 normal = LoadNormalTexture(normalTex, normalTexSampler, input.texCoord0);	
	float3 furNormal = ReconstructNormal(FractalNormal.SampleLevel(FractalNormalSampler, furCoord, 0).zw);
	// furNormal = ScaleNormal(furNormal, 2.0);
	normal = mul(BlendAngleCorrectedNormals(normal, furNormal), tbn);
	
	float3 view = normalize(cameraPosition.xyz - input.worldPosition);
	float vertNov = dot(normal, view);
	float vol = dot(-view, dirLightDirection.xyz);
	// float vertNol = dot(input.worldNormal, dirLightDirection.xyz);

	float radiance = max(dot(normal, dirLightDirection.xyz), 0.0);
	float3 finalColor = lerp(albedo.rgb * 0.2, albedo.rgb, radiance);
	finalColor *= Unorm(1.0 - noise);
	finalColor += Pow4(1.0 - vertNov) * albedo * Unorm(vol);

	static const float luminanceCorrect = 0.5;
	finalColor *= dirLightColor.xyz * dirLightColor.w * luminanceCorrect;
	

	output.color.rgb = finalColor;
	// output.color.rgb = input.color;
	output.color.a = 1.0;

	return output;
}