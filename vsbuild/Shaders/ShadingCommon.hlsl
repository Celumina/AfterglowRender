#ifndef SHADING_COMMON_HLSL
#define SHADING_COMMON_HLSL


float F0RGBToF0(float3 f0) {
	return max(f0.r, max(f0.g, f0.b));
}

float F0ToMircoOcclusion(float f0) {
	return saturate(50.0 * f0);
}

float F0RGBToMircoOcclusion(float3 f0) {
	return F0ToMircoOcclusion(F0RGBToF0(f0));
}

half DielectricSpecularToF0(half specular) {
	// 0.08 is commonly used baseline for dielectric F0 in real-time rendering.
	return half(0.08f * specular);
}

// F0 also known as specularColor.
half3 ComputeF0(half specular, half3 baseColor, half metallic) {
	return lerp(DielectricSpecularToF0(specular).xxx, baseColor, metallic);
}


#endif