
// TODO: Generate Attachment sceneColorTexture info.
// [[vk::input_attachment_index(0)]] SubpassInput<float4> sceneColorTexture;

#include "PostEffects.hlsl"
#include "PostEffectUnderwater.hlsl"
#include "Chromatics/ChromaticsCommon.hlsl"

struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float2 texCoord0 : TEXCOORD0;
};

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(VSOutput input) {
	FSOutput output;

	float2 uniformUV = input.texCoord0;
	uniformUV.x *= screenResolution.x / screenResolution.y;
	int2 pixelCoord = input.texCoord0 * screenResolution;

	float4 color = sceneColorTexture.Sample(sceneColorTextureSampler, input.texCoord0);
	
	// TODO: Check from global marco for MultiSample or sample from sampler.
	float depth = SampleDepth(pixelCoord);
	float sceneDepth = SceneDepth(depth);

	// float4 testTexColor = testTex.Sample(testTexSampler, uniformUV);
	// testTexColor *= testTexColor.w;

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

	// output.color.xyz = WorleyNoise(
	// 	float3(input.texCoord0, time * 0.1), 
	// 	float3(16.0, 16.0, 16.0), 
	// 	randomSeed0
	// );
	// output.color.xyz = InterpolatedNoise(input.texCoord0, float2(16.0, 16.0), randomSeed0);
	// output.color.xyz = SRGBToLinearRGB(output.color.xyz);
	
	// float depthT = SampleDepth(int2(pixelCoord) + int2(0, -1));
	// float depthB = SampleDepth(int2(pixelCoord) + int2(0, 1));
	// float depthL = SampleDepth(int2(pixelCoord) + int2(-1, 0));
	// float depthR = SampleDepth(int2(pixelCoord) + int2(1, 0));

	// float3 edge = (fwidth(depth) + (fwidth(depthT) + fwidth(depthB) +fwidth(depthL) + fwidth(depthR))) * color * 1000;
	//float3 edge = (abs(depth - 0.25 * (depthT + depthB + depthL + depthR))) * color * 8000;

	// Note that here output color was encoded to sRGB by swapchain, so color in screenShot will never be 0.5.
	// Linear render result will be encoded to sRGB by swapchain automatically.
	// output.color.xyz = 0.5;
	output.color = color; //Grayscale(color);
	output.color.xyz = SceneColorChromaticAberration(input.texCoord0, output.color.xyz);
	
	// output.color.xyz = edge.xyz;
	// output.color.xyz = SceneColorLinearBlur(input.texCoord0, sceneDepth);
	// output.color.xyz = sceneDepth;
	
	// Reconstruct world position.
	float3 worldPos = ReconstructWorldPosition(input.texCoord0, depth);

	float3 envVector = normalize(worldPos.xyz - cameraPosition.xyz);
	float normalizePhi = (atan2(envVector.y, envVector.x)) * (0.5 * invPi) + 0.5;
	float normalizeTheta = acos(envVector.z + 1e-2) * invPi;
	float3 envColor = ambientTexture.SampleLevel(ambientTextureSampler, float2(normalizePhi, normalizeTheta), 7).xyz;

	// Height Fog
	float heightFog = HeightFog(worldPos.z, sceneDepth); 
	output.color.xyz = lerp(output.color.xyz, envColor, heightFog);

	// Underwater
	float underwaterMask = UnderwaterMask(input.texCoord0);
	output.color.xyz = lerp(output.color.xyz, float3(0.05, 0.05, 0.15) + envColor * 0.05, saturate(1.0 - exp(-sceneDepth * 0.05 - 0.5)) * underwaterMask);

	// Effect Blending
	output.color.xyz += AtmosphericDust(input.texCoord0, sceneDepth);
	output.color *= Vignette(input.texCoord0);

	// Cursor test.
	output.color += saturate(0.05 - distance(float2(screenResolution.x * invScreenResolution.y, 1.0) * cursorPosition * invScreenResolution, uniformUV));

	output.color.xyz = LUTMapping(output.color.xyz);

	return output;
}