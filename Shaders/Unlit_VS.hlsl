struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float3 worldNormal : NORMAL;
	[[vk::location(2)]] float4 color : COLOR;
	[[vk::location(3)]] float2 texCoord0 : TEXCOORD0;
};

VSOutput main(VSInput input) {
	VSOutput output;
	output.position = mul(projection, mul(view, mul(model, float4(input.position, 1.0))));
	output.worldNormal = mul(invTransModel, float4(input.normal, 0.0)).xyz;
	output.color = input.color;
	output.texCoord0 = input.texCoord0 * float2(1.0, -1.0);
	return output;
}