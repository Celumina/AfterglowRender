#include "Kernels.hlsl"

struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float2 texCoord0 : TEXCOORD0;
};

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(VSOutput input) {
	FSOutput output;
	uint2 maxResolution = resolutionScale * screenResolution;
	uint2 pixelCoord = input.texCoord0 * maxResolution;
	float3 color = 0.0;
	[unroll] for (int index = -7; index <= 7; ++index) {
		uint currentY = clamp(pixelCoord.y + index, 0, maxResolution.y);
		color += bloomHorizontalBlurTexture.Load(uint3(pixelCoord.x, currentY, 0)) * gaussianSigma3[abs(index)];
	}
	// Ignore the first bloomCombinedTexture(due to it is a  downsampling)
	if (useCombinedTexture) {
		color += bloomCombinedTexture.Sample(bloomCombinedTextureSampler, input.texCoord0);
	}
	output.color.xyz = color;
	return output;
}