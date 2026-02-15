#include "EndfieldCommon.hlsl"
#include "../Common.hlsl"
#include "../Depth.hlsl"
#include "../VertexStructs.hlsl"

struct FSOutput {
	float4 color : SV_TARGET;
	float depth : SV_DEPTH;
};

FSOutput main(StandardFSInput input) {
	FSOutput output;
	static const float illuminanceCorrect = 0.5;

	half4 baseColor = albedoTex.Sample(albedoTexSampler, input.texCoord0);
	// half4 mask = maskTex.Sample(maskTexSampler, input.texCoord0);

	// float radiance = smoothstep(0.5, 0.6, faceLightAngle);
	float radiance = max(dot(input.worldNormal, dirLightDirection.xyz), 0.0);

	// Color ramp
	float2 rampCoord = float2(clamp(0.8 - radiance * 0.5, 0.1, 0.9), 0.5);
	half4 rampColor = rampTex.Sample(rampTexSampler, rampCoord);

	float3 finalColor = baseColor.rgb * (radiance + (rampColor.rgb * rampColor.a) * min(subsurfaceMFP, baseColor.xyz));

	// Read scene depth
	float sceneDepth = SampleSceneDepth(int2(input.position.xy));
	float pixelDepth = SceneDepth(input.position.z);

	float depthFade = sceneDepth - pixelDepth;

	finalColor *= dirLightColor.xyz * dirLightColor.w * illuminanceCorrect;

	output.color.xyz = finalColor;
	output.color.a = (saturate((depthFade - eyelashDepthOffset) / eyelashDepthOffset));


	// @note: Writen depth method.
	// float originalPixelDepth = SceneDepth(ZeroNearOneFarDepth(input.color.x / input.color.y));

	// Explicit write depth disable Early-Z. (For scene depth sample the background, instead of itself.)
	// output.depth = input.position.z;

	return output;
}