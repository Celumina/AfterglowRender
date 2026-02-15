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
	static const float illuminanceCorrect = 0.5;

	float3x3 tbn = {
		input.worldTangent, 
		/* note: 
			Some issues were occur in the source mesh tangent, but I was faild to figure it out what's happened, 
			(BlenderSrc: good; BlenderReimportedFBX: good; Unreal: bad; Afterglow: bad; What are blender do for this mesh?)
			So I had to use this trick to fix the tangent issue.
			Note that here input.color stores mesh normalized[0, 1] object position.
		 TODO: Check chirality or try to import bitangent. 
		*/
		input.color.x > 0.5 ? input.worldBitangent : -input.worldBitangent, 
		input.worldNormal
	};
	tbn = input.isFrontFace ? tbn : -tbn;

	half4 baseColor = albedoTex.Sample(albedoTexSampler, input.texCoord0);
	half3 normal = mul(LoadNormalTextureFromXY(normalTex, normalTexSampler, input.texCoord0), tbn);
	float3 view = normalize(cameraPosition.xyz - input.worldPosition);
	float vertNov = dot(input.worldNormal, view);
	float vol = dot(-view, dirLightDirection.xyz);
	float vertNol = dot(input.worldNormal, dirLightDirection.xyz);

	float radiance = max(dot(normal, dirLightDirection.xyz), 0.0);

	// View Attenuation
	float viewFactor = lerp(1.0, max(vertNov, 0.0), viewAttenuation); 
	radiance *= viewFactor;
	float fadedSubsurfaceMFP = subsurfaceMFP * (0.5 + viewFactor * 0.5);

	// Color ramp
	static const float rampOffset = 0.2; // the rampOffset greater, the color more saturation.
	float2 rampCoord = float2(clamp((1.0 - rampOffset) - radiance * (0.5 - (rampOffset * 0.5)), 0.1, 0.9), 0.5);
	half4 rampColor = rampTex.Sample(rampTexSampler, rampCoord);

	float3 finalColor = baseColor.rgb * (radiance + (rampColor.rgb * rampColor.a) * min(fadedSubsurfaceMFP, baseColor.xyz));

	// Rim light
	float3 rimLighting = EndfieldRimLighting(vertNov, vol, vertNol, 1.0, 1.0, rimLightWidth);
	// float rimLightMask = smoothstep(0.0, 0.05, max(Snorm(input.texCoord0.x) * rol, 0.0));
	rimLighting *= rimLightIntensity;

	finalColor += rimLighting;
	finalColor *= dirLightColor.xyz * dirLightColor.w * illuminanceCorrect;

	output.color.xyz = finalColor;
	output.color.a = baseColor.a;

	return output;
}