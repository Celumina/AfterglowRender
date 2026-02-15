
#include "../VertexStructs.hlsl"
#include "../Common.hlsl"

StandardVSOutput main(VSInput input) {
	static const float objectSpaceBiasY = 0.012;
	static const float lightBiasIntensity = 0.0025;
	static const float depthBias = 0.005;

	StandardVSOutput output;
	input.position.y += objectSpaceBiasY;
	input.position.x *= 1.0 + objectSpaceBiasY * 8.0;
	float4 worldPosition = mul(model, float4(input.position, 1.0));

	// TODO: Synchronize cardinals from Cpp.
	float3 forwardDirection = mul(model, float4(0.0, 1.0, 0.0, 0.0)).xyz;
	float3 rightDirection = mul(model, float4(1.0, 0.0, 0.0, 0.0)).xyz;

	float3 projectedLightVector = ProjectVector(dirLightDirection.xyz, forwardDirection);
	float biasFactor = abs(dot(dirLightDirection.xyz, rightDirection));

	worldPosition.xyz -= projectedLightVector * lightBiasIntensity * biasFactor;

	output.position = mul(viewProjection, worldPosition);
	output.position.z -= depthBias / output.position.w;
	output.worldPosition = worldPosition;
	output.worldNormal = normalize(mul(invTransModel, float4(input.normal, 0.0)).xyz);
	// output.worldTangent = normalize(mul(invTransModel, float4(input.tangent, 0.0)).xyz);
	// output.worldBitangent = cross(output.worldNormal, output.worldTangent);
	output.color = input.color;
	output.objectID = objectID; // input.instanceID;
	output.texCoord0 = input.texCoord0;
	return output;
}