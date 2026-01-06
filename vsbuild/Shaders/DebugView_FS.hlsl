#include "VertexStructs.hlsl"
#include "Visualization.hlsl"

#include "Meteorograph/MeteorographCommon.hlsl"

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(UnlitFSInput input) {
	FSOutput output;

	// Meteorograph
	float4 meteorograph = Meteorograph.Sample(MeteorographSampler, input.texCoord0);
	float4 rawMeteorograph = Meteorograph.Load(uint3(input.texCoord0 * 128.0, 0));
	half2 pluviometer = Pluviometer.Sample(PluviometerSampler, input.texCoord0);

	output.color.xyz = float3((meteorograph.w - polarTemperature) * 0.02, meteorograph.z,  pluviometer.x / deltaTime);
	output.color.xyz += Arrow(frac(input.texCoord0 * 128.0), rawMeteorograph.xy * 0.5);

	// World position
	float2 normalizedCameraPos = (cameraPosition.xy) * worldInvSideLength;
	float distToCamera = distance(normalizedCameraPos, input.texCoord0 - 0.5);
	output.color.xyz += distToCamera < 0.004 && distToCamera > 0.0025;

	// Water flow
	// output.color.xyz = 0;

	return output;
}