#ifndef ACES_DISPLAY_ENCODING_HLSL
#define ACES_DISPLAY_ENCODING_HLSL

#include "ACESCommon.hlsl"

// float3 ClampZeroToPeakLuminance(float3 inXYZ, OutputTransformParams odtParams) {
// 	float3 colorRGB = mul(odtParams.limitXYZToRGB, inXYZ);
// 	colorRGB = clamp(colorRGB, 0.0, odtParams.peakLuminance / referenceLuminance);
// 	float3 outXYZ = mul(odtParams.limitRGBToXYZ, colorRGB);
// 	return outXYZ;
// }

// /** TODO: ....
// * @brief: White scaling functioin
// * @return: CIE XYZ color
// */
// float3 ApplyWhiteScaleXYZ(float3 luminanceXYZ, OutputTransformParams odtParams) {
// 	// RGB: Output device-dependent linear color space, which without apply the display encording (EOTF).
// 	float3 whiteRGB = mul(odtParams.outputXYZToRGB, odtParams.limitWhiteXYZ);
// 	float3 normalizedWhiteRGB = (1.0 / referenceLuminance) * whiteRGB;
// 	float scale = 1.0 / max(max(normalizedWhiteRGB.r, normalizedWhiteRGB.g), normalizedWhiteRGB.b);
// 	return scale * ClampZeroToPeakLuminance(luminanceXYZ, odtParams);
// }

#endif