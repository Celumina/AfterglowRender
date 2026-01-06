
#include "ChromaticsCommon.hlsl"
#include "ACESOutputTransform.hlsl"

[numthreads(8, 8, 8)]
void main(uint3 threadID : SV_DispatchThreadID) {
	float4 neutral = float4(lutInvSideLength * threadID, 1.0);

	// Decode log value
	// Working color space: D60-ACEScg (AP1)
	float3 linearColor = LogToLinearColor(neutral.rgb.xyz);// - LogToLinearColor(0);

	// Bake ACES transform into a LUT
	float3 outputColor = OutputTransformFoward(linearColor);

	ColorLUTOut[threadID].xyz = outputColor; 
}