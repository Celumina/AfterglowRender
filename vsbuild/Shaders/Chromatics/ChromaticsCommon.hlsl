#ifndef CHROMATICS_COMMON_HLSL
#define CHROMATICS_COMMON_HLSL

static const uint lutSideLength = 32;
static const float lutInvSideLength = 1.0 / lutSideLength;
static const float lutSampleOffset = 0.5 * lutInvSideLength;

static const float linearColorRange = 14.0;
static const float linearColorGrey = 0.18;
static const float colorExposureGrey = 444.0;

// From GammaCorrectionCommon.ush, Unreal Engine.
float3 LogToLinearColor(float3 logColor) {
	// The maximum linear color value is about 43.708.
	// 1.0(log) -> 43.708 (linear)
	return exp2((logColor - colorExposureGrey / 1023.0) * linearColorRange) * linearColorGrey;

}

float3 LinearToLogColor(float3 linearColor) {
	return saturate(log2(linearColor) / linearColorRange - log2(linearColorGrey) / linearColorRange + colorExposureGrey / 1023.0);
}

float3 LUTMapping(float3 srcColor) {
	// Normalize HDR Color
	return ColorLUTIn.Sample(ColorLUTInSampler, LinearToLogColor(srcColor) + lutSampleOffset).xyz;
}

#endif