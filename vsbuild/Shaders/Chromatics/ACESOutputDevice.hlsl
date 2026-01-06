#ifndef ACES_OUTPUT_DEVICE
#define ACES_OUTPUT_DEVICE

#include "ACESCommon.hlsl"

// TODO: Generate this file from C++.
// Here defined a Rec709-D65-sRGB 100nits as default.

// Display Settings
// Luminance the tone scale highlight rolloff will traget in cd/m^2 (nits)
static const float outputDevicePeakLuminance = 100.0;
// Apply scaling to compress output so that largest channel hits 1.0; usually enabled when using a limiting white different from the encoding white.
// We use D65 as white point.
static const bool outputDeviceScaleWhite = true; 

#define EOTF_ENUM_LINEAR 0
#define EOTF_ENUM_ST_2084 1
#define EOTF_ENUM_HLG 2
#define EOTF_ENUM_GAMMA_2_6 3
#define EOTF_ENUM_BT_1886_WITH_GAMMA_2_4 4
#define EOTF_ENUM_GAMMA_2_2 5
#define EOTF_ENUM_SRGB_IEC_61966_2_1_1999 6

// @unused: Target Device EOTF (Electical-Optical Transfer Function) 
#define OUTPUT_DEVICE_EOTF EOTF_ENUM_SRGB_IEC_61966_2_1_1999

#define GAMUT_ENUM_SRGB_D65 0
#define GAMUT_ENUM_DCIP3_D65 1
#define GAMUT_ENUM_REC2020_D65 2
#define GAMUT_ENUM_ACES_D60 3
#define GAMUT_ENUM_ACESCG_D60 4

#define OUTPUT_DEVICE_GAMUT GAMUT_ENUM_SRGB_D65

static const float3x3 outputDeviceRGBToXYZ = sRGBToXYZMat;
static const float3x3 XYZToOutputDeviceRGB = XYZToSRGBMat;

#endif