#include "../VertexStructs.hlsl"
#include "EndfieldCommon.hlsl"

StandardVSOutput main(VSInput input) {
	StandardVSOutput output;
half4 baseColor = albedoTex.SampleLevel(albedoTexSampler, input.texCoord0, 0);
float3 position = input.position;
// position.y -= baseColor.a * 0.05;

	float4 worldPosition = mul(model, float4(position, 1.0));	

	output.position = mul(viewProjection, worldPosition);
	
	output.worldPosition = worldPosition;
	output.worldNormal = normalize(mul(invTransModel, float4(input.normal, 0.0)).xyz);
	output.worldTangent = normalize(mul(invTransModel, float4(input.tangent, 0.0)).xyz);
	output.worldBitangent = cross(output.worldNormal, output.worldTangent);
	
	// color.x stores tangent.x object space use to distinguish left iris and right iris.
	output.color.xyz = input.tangent.x;

	output.objectID = objectID; // input.instanceID;
	output.texCoord0 = input.texCoord0;
	return output;
}