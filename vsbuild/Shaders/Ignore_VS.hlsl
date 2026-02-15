struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
};

VSOutput main(VSInput input) {
	VSOutput output;
	output.position = 0.0;
	return output;
}