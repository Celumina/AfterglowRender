struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float3 worldPosition : POSITION;
	[[vk::builtin("PointSize")]] float pointSize : PSIZE;
};

struct PSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

PSOutput main(VSOutput input) {
	PSOutput output;
	// Something wrong in this "ErrorShader", but it looks good, reserve it.
	// float value = fmod(input.position, 1.0) > 0.6;
	float3 signs = sign(input.worldPosition);
	bool3 grid = fmod(input.worldPosition, 100.0) < 50.0 * signs;
	bool3 smallGrid = fmod(input.worldPosition, 10.0) < 5.0 * signs;
	float value = 0;
	value = grid.x ^ grid.y ^ grid.z ? 0.75 : value;
	value = smallGrid.x ^ smallGrid.y ^ smallGrid.z ? value + 0.25 : value;
	value += fwidth(input.worldPosition.z) * 0.05;
	output.color = lerp(float4(0.2, 0.0, 0.0, 1.0), float4(0.9, 0.05, 0.05, 1.0), value);
	return output;
}