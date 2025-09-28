struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION; // Screen Space Position
	[[vk::location(1)]] float3 worldPosition : POSITION;
	[[vk::location(2)]] float3 worldNormal : NORMAL;
	[[vk::location(5)]] float4 color : COLOR;
	[[vk::location(6)]] float2 texCoord0 : TEXCOORD0;
	[[vk::location(7)]] uint objectID : OBJECT_ID;
};

VSOutput main(VSInput input) {
	VSOutput output;
	InstanceBufferStruct instanceInfo = InstanceBufferIn[input.instanceID];
	float4x4 instanceModel = {
		instanceInfo.transformR0, 
		instanceInfo.transformR1, 
		instanceInfo.transformR2, 
		instanceInfo.transformR3, 
	};
	float4x4 invTransInstanceModel = transpose(instanceModel);
	invTransInstanceModel = mul(invTransInstanceModel, invTransModel);

	float4 worldPosition = mul(model, float4(input.position, 1.0));

	worldPosition.xy += sin(time + worldPosition.xy * 0.03) * 10.0 * input.color;

	worldPosition = mul(instanceModel, worldPosition);
	output.position = mul(projection, mul(view, worldPosition));
	output.worldPosition = worldPosition;
	output.worldNormal = normalize(mul(invTransInstanceModel, float4(input.normal, 0.0)).xyz);
	output.color = input.color;
	output.objectID = objectID; 
	output.texCoord0 = input.texCoord0 * float2(1.0, -1.0);
	return output;
}