struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float2 texCoord0 : TEXCOORD0;
};

VSOutput main(VSInput input) {
	VSOutput output;
	output.position = float4(input.position.xy, 0.0, 1.0);
	output.texCoord0 = input.texCoord0;
	return output;
}