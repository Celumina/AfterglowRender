struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float2 texCoord0 : TEXCOORD0;
};

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(VSOutput input) {
	FSOutput output;
	output.color = sceneColorTexture.Sample(sceneColorTextureSampler, input.texCoord0);
	return output;
}