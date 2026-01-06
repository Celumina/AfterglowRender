struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float3 velocity : VELOCITY;
	[[vk::location(2)]] float4 color : COLOR;
	[[vk::location(3)]] nointerpolation float2 centerPosition : CENTERPOSITION;
	[[vk::location(4)]] nointerpolation float transferPointSize : TRANSFERPOINTSIZE;
	[[vk::builtin("PointSize")]] nointerpolation float pointSize : PSIZE;
};

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(VSOutput input) {
	FSOutput output;
	output.color.rgb = input.color.rgb;
	float circleAlpha = 1 - distance(input.centerPosition.xy, input.position.xy) / input.transferPointSize * 2;
	// TODO: Find a smoother bell curve.
	circleAlpha = smoothstep(0, 1, circleAlpha);

	// Storage image test.
	// Texture2D.Load(int3(x, y, miplevel));
	output.color.rg = lerp(output.color.rg, TestTextureIn.Load(int3(0, 0, 0)).rg, 0.5);

	float2 screenUV = input.position.xy * invScreenResolution;
	float2 distribUV = ((0.1 + input.centerPosition.xy - input.position.xy) / input.transferPointSize) * (1.0 - circleAlpha) * 0.2;
	float4 sceneColor = sceneColorTexture.Sample(sceneColorTextureSampler, clamp(screenUV + distribUV, invScreenResolution, 1.0 - invScreenResolution));
	output.color.xyz = lerp(sceneColor.xyz, output.color.xyz, circleAlpha * 0.25);
	output.color.a = input.color.a * circleAlpha;

	// output.color = float4(distribUV, 0, 1);
	return output;
}