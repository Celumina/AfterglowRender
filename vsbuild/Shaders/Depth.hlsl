#ifndef DEPTH_HLSL
#define DEPTH_HLSL

#include "Circumstances.hlsl"
#include "Common.hlsl"

float ZeroNearOneFarDepth(float perspectiveDepth) {
	#if REVERSE_DEPTH
	return 1.0 - perspectiveDepth;
	#else
	return perspectiveDepth;
	#endif
}

float LinearDepth(float perspectiveDepth) {
	return (cameraNear * cameraFar) 
		/ (cameraFar + perspectiveDepth * (cameraNear - cameraFar));
}

float SceneDepth(float perspectiveDepth) {
	#if REVERSE_DEPTH
	return LinearDepth(1.0 - perspectiveDepth);
	#else
	return LinearDepth(perspectiveDepth);
	#endif
}

// @return: Raw depth from depth buffer (perpective)
float SampleDepth(float2 uv) {
	#if ENABLE_MSAA
	return depthTexture.Load(uv * screenResolution, 0);
	#else
	return depthTexture.Sample(depthTextureSampler, uv);
	#endif
}

// @return: Raw depth from depth buffer (perpective)
float SampleDepth(int2 pixelCoord) {
	#if ENABLE_MSAA
	return depthTexture.Load(pixelCoord, 0);
	#else
	return depthTexture.Sample(depthTextureSampler, pixelCoord * invScreenResolution);
	#endif
}

template<typename CoordType>
float SampleSceneDepth(CoordType xy) {
	return SceneDepth(SampleDepth(xy));
}

float3 ReconstructWorldPosition(float2 uv, float perspectiveDepth) {
	float4 worldPos = mul(invViewProjection, float4(Snorm(uv), ZeroNearOneFarDepth(perspectiveDepth), 1.0));
	return worldPos.xyz / worldPos.w;
}

#endif