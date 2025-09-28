struct VSOutput {
	[[vk::location(0)]] float4 position : SV_POSITION; // Screen Space Position
	[[vk::location(1)]] float3 worldPosition : POSITION;
	[[vk::location(2)]] float3 worldNormal : NORMAL;
	[[vk::location(3)]] float3 worldTangent : TANGENT;
	[[vk::location(4)]] float3 worldBitangent : BITANGENT;
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
	// invTransInstanceModel[2] = float4(0.0, 0.0, 0.0, 0.0);
	invTransInstanceModel = mul(invTransInstanceModel, invTransModel);

	float4 worldPosition = mul(model, float4(input.position, 1.0));
	worldPosition = mul(instanceModel, worldPosition);
	output.position = mul(projection, mul(view, worldPosition));
	output.worldPosition = worldPosition;
	output.worldNormal = normalize(mul(invTransInstanceModel, float4(input.normal, 0.0)).xyz);
	output.worldTangent = normalize(mul(invTransInstanceModel, float4(input.tangent, 0.0)).xyz);
	output.worldBitangent = cross(output.worldNormal, output.worldTangent);
	output.color = input.color;
	output.objectID = objectID; 
	output.texCoord0 = input.texCoord0 * float2(1.0, -1.0);
	return output;
}