#include "../ShadingModels.hlsl"
#include "../Random.hlsl"
#include "../Depth.hlsl"
#include "TerrainCommon.hlsl"

struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float4 worldPosition : POSITION;
	bool isFrontFace : SV_ISFRONTFACE;
};

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(VSOutput input) {
	static const float edgeFadeDistance = 0.4;

	FSOutput output;

	float waterSediment = SampleTerrain(WaterSediment, WaterSedimentSampler, input.worldPosition.xy).x;
	float2 waterVelocity = SampleTerrain(WaterVelocity, WaterVelocitySampler, input.worldPosition.xy);
	float waterSpeedFactor = saturate(length(waterVelocity) * 16.0);
	
	float3 viewDir = normalize(cameraPosition.xyz - input.worldPosition);

	// Velocity Noise Tex
	float2 baseNoiseUV = input.worldPosition.xy * 0.05;
	float2 noiseVelocityOffset = clamp(waterVelocity, -0.1, 0.1) * 2.0;
	// @note: If foam lagging, try to increase this value (decrease cycle time).
	float noiseTimeScale = 0.5;
	float noiseTime = time * noiseTimeScale;
	float fractalSimplexA = FractalSimplexNoise.Sample(FractalSimplexNoiseSampler, baseNoiseUV - noiseVelocityOffset * frac(noiseTime));
	float fractalSimplexB = FractalSimplexNoise.Sample(FractalSimplexNoiseSampler, baseNoiseUV + 0.5 - noiseVelocityOffset * frac(noiseTime + 0.5));
	// SimplexC for variaint shapes.
	float fractalSimplex = lerp(fractalSimplexA, fractalSimplexB, abs(Snorm(frac(noiseTime))));
	float fractalSimplexC = FractalSimplexNoise.Sample(FractalSimplexNoiseSampler, baseNoiseUV * 0.5 + noiseTime * 0.025 + fractalSimplex * 0.5);
	fractalSimplex = saturate((min(fractalSimplex, fractalSimplexC - 0.075) - 0.25) * 6.0);

	// Water normal: Dither coord offset to reduce aliasing.
	float2 coordOffset = Snorm(Hash2D(ceil((input.position.xy + input.worldPosition.xy) * 128.0) * 0.001, randomSeed0)) * 0.35;
	float3 normal = ReconstructNormal(SampleTerrain(TerrainNormal, TerrainNormalSampler, input.worldPosition.xy + coordOffset).zw);
	normal = lerp(normal, normalize(float3(normal.xy * 2.0, normal.z)), waterSpeedFactor);

	// TODO: More accurate model
	// TODO: Culling scene color uv which Higher than surface
	// Here 0.1 limits the edge clamping.
	float2 refractedUVOffset = normal.xy * min(input.position.z, 0.1) * 5.0;

	// @note: A mesh-shaped artifact from MSAA resolve attechment is solved by
	// 	disabling depthWhite of the alpha object. 
	// 	It was caused due to the MSAA is designed for Opaque object, it coverage and depth min
	// 	could affect the transparency (color.w).
	float sceneDepth = SampleSceneDepth(int2(clamp(input.position.xy + refractedUVOffset * screenResolution, 0.0, screenResolution - 1.0)));
	float pixelDepth = SceneDepth(input.position.z);
	float depthFade = sceneDepth - pixelDepth;

	float2 refractedUV = input.position.xy * invScreenResolution;
	// TODO: Don't use depth fade, use water delta height instead.
	refractedUV += refractedUVOffset * depthFade;
	refractedUV = clamp(refractedUV, invScreenResolution, 1.0 - invScreenResolution);
	half3 sceneColor = sceneColorTexture.Sample(sceneColorTextureSampler, refractedUV);

	// Depth color
	half4 baseColor = lerp(
		half4(0.25, 0.4, 0.4, 1.0),
		half4(0.01, 0.1, 0.3, 1.0), 
		clamp(1 - exp(-depthFade * 0.02), 0.0, 1.0)
	);
	
	// Sediment
	// TODO: Sediment affects the depth attenuation.
	baseColor = lerp(
		baseColor, 
		half4(0.4, 0.3, 0.2, 1.0), 
		saturate(waterSediment * waterInvSedimentCapability * 100.0)
	);

	float matallic = lerp(0.15, 0.1, waterSpeedFactor);
	float specular = lerp(0.5, 0.1, waterSpeedFactor);
	float roughness = lerp(0.1, 0.3, waterSpeedFactor);

	// TODO: normal foam
	// baseColor = lerp(baseColor, half4(0.9, 0.9, 0.9, 1.0), length(normal.xy));

	ShadingContext shadingContext = (ShadingContext)0;
	shadingContext.baseColor = baseColor.xyz;
	shadingContext.metallic = input.isFrontFace ? matallic : 0.0;
	shadingContext.specular = input.isFrontFace ? specular : 0.01;
	shadingContext.roughness = roughness;
	shadingContext.ambientOcclusion = 0.5;
	shadingContext.normal = normal; //input.worldNormal; 
	shadingContext.view = viewDir;

	LightingResult lightingResult = DefaultShading(shadingContext);
	float3 finalColor = lightingResult.diffuse + lightingResult.specular + lightingResult.transmission;

	// Depth attenuation
	float deltaHeightToCamera = cameraPosition.z - input.worldPosition.z;
	float attenuation = 
		input.isFrontFace 
		? clamp(1.0 - exp(-depthFade * 0.2), 0.0, 1.0)
		: clamp(-deltaHeightToCamera * 0.01 + 0.5, 0.8, 1.0);

	attenuation -= 1.0 - clamp(depthFade, 0.0, edgeFadeDistance) * (1.0 / edgeFadeDistance);
	attenuation = max(attenuation, 0.0);
	// Camera fade for underwater
	float cameraHorizontality = pow(length(cameraVector.xy), 0.2);
	attenuation *= saturate(9.0 + 6.0 * log(length(input.worldPosition.xyz - cameraPosition.xyz)) * cameraHorizontality);

	output.color.xyz = lerp(sceneColor.xyz, finalColor, attenuation);

	// Velocity
	output.color.xyz = lerp(output.color.xyz, half4(0.7, 0.7, 0.75, 1.0), waterSpeedFactor * fractalSimplex);

	// output.color.xyz = distortedDepthFade * 0.1;

	output.color.w = 1.0;

	return output;
}