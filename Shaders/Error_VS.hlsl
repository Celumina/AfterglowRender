struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float3 worldPosition : POSITION;
	[[vk::builtin("PointSize")]] float pointSize : PSIZE;
};

VSOutput main(VSInput input) {
	VSOutput output;
	float4 worldPosition = mul(model, float4(input.position, 1.0));
	output.position = mul(projection, mul(view, worldPosition));
	output.worldPosition = worldPosition;
	output.pointSize = 0.0;
	return output;
}