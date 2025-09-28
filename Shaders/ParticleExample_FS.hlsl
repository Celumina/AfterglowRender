struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float4 velocity : VELOCITY;
	[[vk::location(2)]] float4 color : COLOR;
	[[vk::location(3)]] nointerpolation float2 centerPosition : CENTERPOSITION;
	[[vk::location(4)]] nointerpolation float transferPointSize : TRANSFERPOINTSIZE;
	[[vk::builtin("PointSize")]] nointerpolation float pointSize : PSIZE;
};

struct PSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

PSOutput main(VSOutput input) {
	PSOutput output;
	output.color.rgb = input.color.rgb;
	output.color.a = 1 - distance(input.centerPosition.xy, input.position.xy) / input.transferPointSize * 2;
	// TODO: Find a smoother bell curve.
	output.color.a = smoothstep(0, 1, output.color.a);
	
	float2 screenUV = input.position.xy / screenResolution;
	float2 distribUV = (input.centerPosition.xy - input.position.xy) / input.transferPointSize * 0.05;
	float4 sceneColor = sceneColorTexture.Sample(sceneColorTextureSampler, screenUV + distribUV);
	output.color.xyz = lerp(sceneColor.xyz, output.color.xyz, output.color.a * 0.5);
	return output;
}