struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float2 texCoord0 : TEXCOORD0;
};

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(VSOutput input) {
	FSOutput output;
	// Low quality
	// float2 texCoord = input.texCoord0 + (invScreenResolution * resolutionInvScale * 0.5); 
	// output.color.xyz = downSampledTexture.SampleLevel(downSampledTextureSampler, texCoord, 0);

	// High quality
	float3 color = 0.0;
	float2 invExtent = invScreenResolution * resolutionInvScale;
	float2 texCoords[4] = {
		input.texCoord0 + invExtent * float2(-0.5, -0.5), 
		input.texCoord0 + invExtent * float2( 0.5, -0.5),
		input.texCoord0 + invExtent * float2(-0.5,  0.5),
		input.texCoord0 + invExtent * float2( 0.5,  0.5)
	};
	[unroll] for(uint index = 0; index < 4; ++index) {
		color += downSampledTexture.SampleLevel(downSampledTextureSampler, texCoords[index], 0);
	}
	output.color.xyz = color * 0.25;
	
	return output;
}