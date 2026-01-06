struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION;
	[[vk::location(1)]] float3 velocity : VELOCITY;
	[[vk::location(2)]] float4 color : COLOR;
	[[vk::location(3)]] nointerpolation float2 centerPosition : CENTERPOSITION;
	[[vk::location(4)]] nointerpolation float transferPointSize : TRANSFERPOINTSIZE;
	[[vk::builtin("PointSize")]] nointerpolation float pointSize : PSIZE;
};

VSOutput main(VSInput input) {
	// Successfully.
	// float4 accessSSBOTest = ParticleSSBOIn[0].color;
	VSOutput output;
	output.position = mul(viewProjection, float4(input.position.xyz, 1.0));
	output.velocity = input.velocity;
	output.color = input.color;
	
	output.centerPosition = (output.position.xy / output.position.w * 0.5 + 0.5) * screenResolution;
	// PointSize: radius of point.
	const static float pointSize = 0.08;
	output.pointSize = pointSize / output.position.w * screenResolution.y / cameraFov;
	output.transferPointSize = output.pointSize;
	return output;
}
