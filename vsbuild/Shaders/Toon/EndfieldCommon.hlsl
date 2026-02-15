#ifndef ENDFIELD_COMMON_HLSL
#define ENDFIELD_COMMON_HLSL

#include "../Common.hlsl"

static const float eyelashDepthOffset = 0.015;

float3 EndfieldRimLighting(float nov, float vol, float nol, float smoothness, float occlusion, float width) {
	float rimStepMin = clamp(0.9 - width, 0.0, 0.99);
	float rimStepMax = clamp(1.0 - width, 0.01, 1.0);
	float rimLighting = smoothness * smoothstep(rimStepMin, rimStepMax, 1.0 - nov);
	// Light dir factor
	// nol + 0.5 for wrap light around.
	rimLighting *= max(vol, 0.0) * max(nol + 0.5, 0.0) * 2.0;
	// Light radiance factor
	rimLighting *= dirLightColor.xyz * dirLightColor.z * occlusion;
	return rimLighting;
}

float3 EndfieldRimLighting(float3 vertNormal, float3 view, float smoothness, float occlusion, float width) {
	float nov = dot(vertNormal, view);
	float vol = dot(-view, dirLightDirection.xyz);
	float nol = dot(vertNormal, dirLightDirection.xyz);
	return EndfieldRimLighting(nov, vol, nol, smoothness, occlusion, width);
}


#endif