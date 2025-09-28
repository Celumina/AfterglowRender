
// TODO: Generate Attachment sceneColorTexture info.
// [[vk::input_attachment_index(0)]] SubpassInput<float4> sceneColorTexture;

#include "ColorConversion.hlsl"
#include "Random.hlsl"

struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float2 texCoord0 : TEXCOORD0;
};

struct PSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

PSOutput main(VSOutput input) {
	PSOutput output;

	float2 uniformUV = input.texCoord0;
	uniformUV.x *= screenResolution.x / screenResolution.y;
	int2 screenPos = input.texCoord0 * screenResolution;

	float4 color = sceneColorTexture.Sample(sceneColorTextureSampler, input.texCoord0);
	float depth = depthTexture.Load(screenPos, 0);

	float4 testTexColor = testTex.Sample(testTexSampler, uniformUV);
	testTexColor *= testTexColor.w;
	

	float depthWeight = max(0.0, (depth - 0.999) * 1000.0);
	// float focuWeight = pow(abs(depthWeight - 0.1), 2.0) + Pow4(0.8 - depthWeight);
	float focuWeight = saturate(Pow5(depthWeight) + Pow5(100.0 * (1.0 - depth)));
	// output.color = lerp(output.color, Grayscale(output.color), depthWeight);
	
	// Average blur
	float4 avgBlur = 0.0;
	int kernelWidth = int(12.0 * focuWeight) * 2 + 2;
	// static const int kernelWidth = 24;
	static const int loadInterval = 4;
	for (int indexX = -kernelWidth / 2 + 1; indexX < kernelWidth / 2; ++indexX) {
		for (int indexY = -kernelWidth / 2 + 1; indexY < kernelWidth / 2; ++indexY) {
			// Box Filter
			float boxWeight = (1.0 / float(kernelWidth * kernelWidth));
			// Tent Filter
			float2 tent = float2(kernelWidth - abs(int2(indexX, indexY))) / (float(kernelWidth) - 0.5);
			float tentWeight = tent.x * tent.y * boxWeight * 2;
			// float interestingWeight = (((kernelWidth / 4) - abs(indexX))) * (((kernelWidth / 4) - abs(indexY))) * boxWeight;
			avgBlur += sceneColorTexture.Load(uint3(screenPos + loadInterval * int2(indexX, indexY), 0)) * tentWeight;

		}
	}

	// // Hash blur
	// float2 hashValue = (float2(
	// 	PseudoRandom(screenResolution.xy * input.texCoord0.yx), 
	// 	PseudoRandom(screenResolution.yx * input.texCoord0.xy)
	// ) * 2.0 - 1.0) * 0.1 * focuWeight;
	// float2 hashUV = clamp(input.texCoord0 + hashValue, 0.0, 1.0);
	// float hashDepth = depthTexture.Load(screenResolution * hashUV, 0);
	// // Handle unexpected blur sample.
	// hashUV = hashDepth + 0.0001 < depth ? input.texCoord0 : hashUV;
	// float4 hashBlur = sceneColorTexture.Sample(sceneColorTextureSampler, hashUV);
	// // float2 randomMultiplier = float2(Hash(input.texCoord0.x * 0.3523 + input.texCoord0.y), Hash(input.texCoord0.y * 0.52345 + input.texCoord0.x));
	// output.color = hashBlur;

	// output.color.xyz = 0.0;
	// static const uint fractualCount = 16;
	// static const float invFractualCount = 1.0 / float(fractualCount);
	// [Unroll] for(uint index = 0; index < fractualCount; ++index) {
	// 	float weight = Snorm(float(fractualCount - index) * invFractualCount * 2.0);
	// 	output.color.xyz += SimplexNoise(
	// 		float3(input.texCoord0, time * 0.15 * weight), 
	// 		pow(index, 1.6) * 0.5, 
	// 		randomSeed0
	// 	) * invFractualCount * weight;
	// }
	// output.color.xyz = WorleyNoise(
	// 	float3(input.texCoord0, time * 0.1), 
	// 	float3(16.0, 16.0, 16.0), 
	// 	randomSeed0
	// );
	// output.color.xyz = InterpolatedNoise(input.texCoord0, float2(16.0, 16.0), randomSeed0);
	// output.color.xyz = SRGBToLinearRGB(output.color.xyz);

	
	float depthT = depthTexture.Load(int2(screenPos) + int2(0, -1), 0);
	float depthB = depthTexture.Load(int2(screenPos) + int2(0, 1), 0);
	float depthL = depthTexture.Load(int2(screenPos) + int2(-1, 0), 0);
	float depthR = depthTexture.Load(int2(screenPos) + int2(1, 0), 0);

	// float3 edge = (fwidth(depth) + (fwidth(depthT) + fwidth(depthB) +fwidth(depthL) + fwidth(depthR))) * color * 1000;
	float3 edge = (abs(depth - 0.25 * (depthT + depthB + depthL + depthR))) * color * 8000;

	// Note that here output color was encoded to sRGB by swapchain, so color in screenShot will never be 0.5.
	// Linear render result will be encoded to sRGB by swapchain automatically.
	// output.color.xyz = 0.5;
	output.color = color;
	// output.color.xyz = edge.xyz;

	// Cursor test.
	output.color += saturate(0.05 - distance(float2(screenResolution.x * invScreenResolution.y, 1.0) * cursorPosition * invScreenResolution, uniformUV));

	return output;
}