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

	half4 baseColor = albedoTex.Sample(albedoTexSampler, input.texCoord0);
	half4 mask = maskTex.Sample(maskTexSampler, input.texCoord0);
	float3 view = normalize(cameraPosition.xyz - input.worldPosition);
	float vertNov = dot(input.worldNormal, view);
	float vol = dot(-view, dirLightDirection.xyz);
	float vertNol = dot(input.worldNormal, dirLightDirection.xyz);

	// @note: Here we working on static mesh, so just use object directions, 
	// 	 if it's skeletal mesh case, use head directions instead.
	// Project light dir into plane which is perpendicular to the objectUp.
	float3 projectedLightDir = ProjectDirection(dirLightDirection.xyz, objectUp.xyz);
	float rol = dot(projectedLightDir.xyz, objectRight.xyz);
	float2 sdfCoord = input.texCoord0;
	if (rol > 0.0) {
		sdfCoord.x = 1.0 - sdfCoord.x;
	}
	half4 sdf = sdfTex.Sample(sdfTexSampler, sdfCoord);

	float fol = dot(dirLightDirection.xyz, objectForward.xyz);
	float normalizedlightAngle = atan2(rol, -fol) * (invPi);
	float lightThreadshold = abs(normalizedlightAngle);
	
	// Threadshold method: 
	// 	abs(normalizedlightAngle) < (sdf.x + sdf.y) * 0.5
	// Smooth method:
	float faceLightAngle = Unorm(((sdf.x + sdf.y) * 0.5) - abs(normalizedlightAngle));
	float faceRadiance = smoothstep(0.5, 0.6, faceLightAngle);
	faceRadiance = lerp(faceRadiance, max(dot(input.worldNormal, dirLightDirection.xyz), 0.0), mask.y);

	// view attenuation 
	float viewFactor = lerp(1.0, max(vertNov, 0.0), viewAttenuation * mask.x); 
	faceRadiance *= viewFactor;
	float fadedSubsurfaceMFP = subsurfaceMFP * (0.5 + viewFactor * 0.5);

	// Color ramp
	static const float rampOffset = 0.2; // the rampOffset greater, the color more saturation.
	float2 rampCoord = float2(clamp((1.0 - rampOffset) - faceRadiance * (0.5 - (rampOffset * 0.5)), 0.1, 0.9), 0.5);
	half4 rampColor = rampTex.Sample(rampTexSampler, rampCoord);

	float3 finalColor = baseColor.rgb * (faceRadiance + (rampColor.rgb * rampColor.a) * min(fadedSubsurfaceMFP, baseColor.xyz));

	// Nose color 
	finalColor = lerp(finalColor * 0.5, finalColor, baseColor.a);

	// Face rim lighting
	// Rim light
	float3 rimLighting = EndfieldRimLighting(vertNov, vol, vertNol, 1.0, mask.w, rimLightWidth);
	float rimLightMask = smoothstep(0.0, 0.01, max(Snorm(input.texCoord0.x) * rol, 0.0));
	rimLightMask *= rimLightIntensity;

	// Lip highlight
	// @note: Offset the uv to simulate shift of highlight.  
	half4 highlightMask = highlightMaskTex.Sample(highlightMaskTexSampler, float2(input.texCoord0.x - rol * 0.025, input.texCoord0.y));
	float lipHighlihgt = highlightMask.r * faceRadiance;

	finalColor += rimLighting * rimLightMask;
	finalColor *= dirLightColor.xyz * dirLightColor.w * illuminanceCorrect;
	finalColor += lipHighlihgt;

	output.color.xyz = finalColor;
	// output.color.xyz = mask.x; 
	
	output.color.a = baseColor.a;

	return output;
}