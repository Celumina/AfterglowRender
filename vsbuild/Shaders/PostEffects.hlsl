#ifndef POST_EFFECTS_HLSL
#define POST_EFFECTS_HLSL


#include "Depth.hlsl"
#include "ColorConversion.hlsl"
#include "Random.hlsl"

// TODO: Depth could be reversed, Create a marco header to read.
// TODO: Seperate fore, mid and background.
// TODO: Just for test, calculate blur from ComputeShader to improve performance.
float3 SceneColorLinearBlur(float2 uv, float sceneDepth) {
	float depthWeight = sceneDepth / cameraFar;
	float focuWeight = saturate(depthWeight * 10.0);

	int2 pixelCoord = uv * screenResolution;
	// Average blur
	float3 avgBlur = 0.0;
	int kernelWidth = int(6.0 * focuWeight) * 2 + 2;
	// static const int kernelWidth = 24;
	static const int loadInterval = 4;
	for (int indexX = -kernelWidth / 2 + 1; indexX < kernelWidth / 2; ++indexX) {
		for (int indexY = -kernelWidth / 2 + 1; indexY < kernelWidth / 2; ++indexY) {
			// Box Filter
			float boxWeight = (1.0 / (float(kernelWidth * kernelWidth) - 0.5));
			// Tent Filter
			float2 tent = float2(kernelWidth - abs(int2(indexX, indexY))) / (float(kernelWidth) - 0.5);
			float tentWeight = tent.x * tent.y * boxWeight * 2;
			avgBlur += 
				sceneColorTexture.Load(uint3(pixelCoord + loadInterval * int2(indexX, indexY), 0))
				* tentWeight;
		}
	}
	return avgBlur;
}

float3 SceneColorChromaticAberration(float2 uv, float3 sceneColor, float intensity = 0.0025) {
	float2 uvOffset =  (uv - float2(0.5, 0.5)) * intensity;
	// float3 sceneColorR = sceneColorTexture.Sample(uv).r;
	float sceneColorG = sceneColorTexture.Sample(sceneColorTextureSampler, uv - uvOffset).g;
	float sceneColorB = sceneColorTexture.Sample(sceneColorTextureSampler, uv - uvOffset * 2).b;

	return float3(sceneColor.r, sceneColorG, sceneColorB);

}

float3 AtmosphericDust(float2 uv, float sceneDepth, float intensity = 0.4) {
	// For fixed time rate
	float timeSeed = frac(floor(time * 20.0) * 0.05);
	float3 baseNoise = Snorm(Desaturation(Hash3D(float3(uv, timeSeed), randomSeed0), 0.5));
	baseNoise = any(abs(baseNoise) > intensity) ? 0 : baseNoise;
	return baseNoise * min((sceneDepth * 0.001), 0.1);
}

float Vignette(float2 uv, float intensity = 0.75, float hardness = 0.5) {
	hardness *= 0.5;
	return 1.0 - smoothstep(min(hardness, 0.5), max(1.0 - hardness, 0.501), saturate(distance(0.5, uv) * intensity));
}

float HeightFog(float worldHeight, float sceneDepth, float distanceFade = 0.25, float intensity = 0.4) {
	float heightFog = Square(max(cameraPosition.z - worldHeight + sceneDepth * distanceFade, 0.0) * 0.01) * intensity;
	// Exponent fadeoff: 1.0 - exp(-k * x); 
	return saturate(1.0 - exp(-2.0 * heightFog));	
}

float3 SkylightShaft(float3 bloomColor, float2 uv, float sceneDepth, float intensity = 0.25, uint layerCount = 8) {
	// Auto offset value and hash weight
	float invLayerCount = rcp(layerCount);
	float layerInterval = 0.32 * invLayerCount;
	float hashScale = 0.16 * invLayerCount;// 0.32 * invLayerCount;
	float3 radialBlurColor = 0.0;
	float3 dirLightDirectionCS = mul(view, dirLightDirection).xyz;
	float2 radialBlurOffset = float2(layerInterval, -layerInterval) * dirLightDirectionCS.xy * (1.0 - dirLightDirectionCS.z) + Snorm(uv) * min(dirLightDirectionCS.z, 0.0) * 0.02;
	float2 screenLightDirection = normalize(dirLightDirectionCS.xy);
	for (int index = 1; index < layerCount + 1; ++index) {
		float2 radialBlurUV = uv + index * radialBlurOffset;
		float2 radialHash = hashScale * Snorm(Hash2D(uv + index, frac(time) * randomSeed0)) * screenLightDirection;
		radialBlurUV += radialHash;
		radialBlurUV = clamp(radialBlurUV, invScreenResolution, 1.0 - invScreenResolution);
		// Color sample was replaced by the bloom color.
		radialBlurColor += 
			// sceneColorTexture.SampleLevel(sceneColorTextureSampler, radialBlurUV, 0).xyz
			bloomColor 
			* (SampleDepth(radialBlurUV) < 0.0001).x;
	}
	radialBlurColor *= invLayerCount;
	return radialBlurColor * intensity * min(sceneDepth * 0.5, 1.0);

}

#endif