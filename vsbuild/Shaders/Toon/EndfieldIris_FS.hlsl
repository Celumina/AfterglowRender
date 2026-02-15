#include "../Common.hlsl"
#include "../ShadingModels.hlsl"
#include "../VertexStructs.hlsl"

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(StandardFSInput input) {
	FSOutput output;
	static const float illuminanceCorrect = 0.5;

	// @note: here tbn actually are transpose(tbn)
	float3x3 tbn = {
		input.worldTangent, 
		input.worldBitangent, 
		input.worldNormal
	};
	// if (input.color.x > 0) {
	// 	tbn[0] *= -1;
	// }
	
	// tbn = input.isFrontFace ? tbn : -tbn;

	// half3 normal = mul(LoadNormalTextureFromXY(normalTex, normalTexSampler, input.texCoord0), tbn);
	// float3 view = normalize(cameraPosition.xyz - input.worldPosition);
	// float vertNov = dot(input.worldNormal, view);
	// float vol = dot(-view, dirLightDirection.xyz);
	// float vertNol = dot(input.worldNormal, dirLightDirection.xyz);

	// float radiance = max(dot(normal, dirLightDirection.xyz), 0.0);

	// // View Attenuation
	// float viewFactor = lerp(1.0, max(vertNov, 0.0), viewAttenuation); 
	// radiance *= viewFactor;
	// float fadedSubsurfaceMFP = subsurfaceMFP * (0.5 + viewFactor * 0.5);

	// // Color ramp
	// float2 rampCoord = float2(clamp(0.8 - radiance * 0.5, 0.1, 0.9), 0.5);
	// half4 rampColor = rampTex.Sample(rampTexSampler, rampCoord);

	// float3 finalColor = baseColor.rgb * (radiance + (rampColor.rgb * rampColor.a) * min(fadedSubsurfaceMFP, baseColor.xyz));

	// // Rim light
	// float3 rimLighting = EndfieldRimLighting(vertNov, vol, vertNol, 1.0, 1.0, rimLightWidth);
	// // float rimLightMask = smoothstep(0.0, 0.05, max(Snorm(input.texCoord0.x) * rol, 0.0));
	// rimLighting *= rimLightIntensity;

	// finalColor += rimLighting;
	// finalColor *= dirLightColor.xyz * dirLightColor.w * illuminanceCorrect;

	float irisUVOffset = (1.0 - distance(input.texCoord0, 0.5) * 2.0) * irisDepth;
	float3 tangentCameraToPos = mul(tbn, normalize(input.worldPosition - cameraPosition.xyz));
	float2 irisViewOffset = {tangentCameraToPos.x, -tangentCameraToPos.y};

	irisViewOffset.y = input.color.x > 0 ? irisViewOffset.y : -irisViewOffset.y;
	float2 irisUV = input.texCoord0 + (irisViewOffset * (irisUVOffset));
	
	// Iris
	half4 baseColor = albedoTex.Sample(albedoTexSampler, irisUV);

	// Corneal
	// Corneal tangent space normal
	float3 cornealNormalTS = ReconstructNormal(Snorm(input.texCoord0));
	cornealNormalTS.y = input.color.x < 0 ? cornealNormalTS.y : -cornealNormalTS.y;
	// Corneal world space normal
	float3 cornealNormal = mul(cornealNormalTS, tbn);
	// Corneal camera space normal
	float3 cornealNormalCS = mul(view, float4(cornealNormal, 0.0)).xyz;

	// Iris lighting
	// Iris world space normal
	float3 irisNormal = float3(-cornealNormal.xy, cornealNormal.z);
	static const float minRadiance = 0.25;
	float irisRadiance = max(dot(dirLightDirection.xyz, irisNormal), max(minRadiance, baseColor.a));

	half4 cornealMatcap = cornealMatcapTex.Sample(cornealMatcapTexSampler, Unorm(-cornealNormalCS.xy));
	half4 irisMatcap = irisMatcapTex.Sample(irisMatcapTexSampler, irisUV);
	float3 finalColor = baseColor.xyz * irisRadiance + irisMatcap.xyz;
	
	float roughness = 0.0005;

	static const float3 minViewZ = -0.5;
	float3 view = cameraPosition.xyz - input.worldPosition.xyz;
	view.z = min(minViewZ, view);
	view = normalize(view);
	float3 halfVec = dirLightDirection.xyz + view;
	halfVec /= length(halfVec);
	float noh = dot(cornealNormal, halfVec);
	float nov = dot(cornealNormal, view);

	// Corneal specular
	float ggx = DistributionGGX(roughness, noh);
	ggx = smoothstep(0.0, 2.0, ggx) * (1.0 - baseColor.a);
	finalColor += ggx;

	// Env reflection
	static const float envLightingIntensity = 0.5;
	half3 reflectionVector = reflect(-view, cornealNormal);
	float normalizePhi = (atan2(reflectionVector.y, reflectionVector.x)) * (0.5 * invPi) + 0.5;
	float normalizeTheta = acos(reflectionVector.z) * invPi;
	float3 envColor = ambientTexture.SampleLevel(ambientTextureSampler, float2(normalizePhi, normalizeTheta), 6).xyz;
	finalColor += envColor * Unorm(cornealMatcap) * Unorm(1.0 - nov) * envLightingIntensity;

	// finalColor = Unorm(1.0 - nov);

	// TODO: Abstract Light function.
	finalColor *= dirLightColor.xyz * dirLightColor.w * illuminanceCorrect;

	output.color.xyz = finalColor;//dot(cornealNormal, dirLightDirection.xyz);
	output.color.a = 1.0;

	return output;
}