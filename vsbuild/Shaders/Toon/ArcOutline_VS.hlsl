#include "../VertexStructs.hlsl"

UnlitVSOutput main(VSInput input) {
	UnlitVSOutput output;
	// input.position = floor(input.position * 0.25) * 4.0;
	float4 worldPosition = mul(model, float4(input.position, 1.0));
	float3 worldNormal = normalize(mul(invTransModel, float4(input.normal, 0.0)).xyz);

	// Expand world position by normal.
	float distanceFade = min(distance(cameraPosition.xyz, worldPosition.xyz) * outlineDistanceFadeFactor, 0.01);
	float fovFade = invProjection[0][0] * invScreenAspectRatio;
	float3 faceFadedDir = (worldPosition.xyz - cameraPosition.xyz) * (1.0 - input.color.b);
	float3 suspensionFadedDir = worldNormal * input.color.a;
	worldPosition.xyz += outlineWidth * (distanceFade * fovFade) * (faceFadedDir + suspensionFadedDir);
	// worldPosition.xyz += worldNormal * 0.01;

	output.position = mul(viewProjection, worldPosition);
	output.texCoord0 = input.texCoord0;
	return output;
}