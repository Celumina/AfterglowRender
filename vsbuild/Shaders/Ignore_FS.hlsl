struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
};

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(VSOutput input) {
	clip(-1);
	FSOutput output;
	return output;
}