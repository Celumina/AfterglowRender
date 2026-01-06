#ifndef ACES_HLSL
#define ACES_HLSL

#include "../Common.hlsl"
#include "ACESTableCommon.hlsl"
#include "ACESTonescale.hlsl"
#include "ACESDisplayEncoding.hlsl"
#include "ACESOutputDevice.hlsl"

/** ACES2 Chromatic encoding and decoding utilities
* Workflow:
* 	InputTransform (IDT) -> LookTransform (LMT) -> [OutputTransform (ODT): RenderingTransform -> DisplayEncoding]
* 	
* ColorSpace Transform:
* 	InputTransform (IDT): InputDeviceFormat (e.g Camera Formats...) -> ACES2065-1 (AP0)
* 	LookTransform (LMT): ACES2065-1 (AP0) -> ACES2065-1 (AP0)
* 	OutputTransform (ODT): ACEScg (AP1) -> jMh -> OutputDeviceFormat (e.g sRGB, Rec.709, DCI-P3...)
* 	
* In Compute Graphics, we focus on the OutputTranform, It can be breakdown into these procedures: 
* 	ACES RGB Input(ACEScg, AP1) -> 
* 	ACES to JMh (lightness, colorfulness, hue) -> 
* 	Tonescale (For lightness) -> 
* 	ChromaCompression (For colorfulness) ->
* 	GamutCompression (For lightness and colorfulness) ->
* 	WriteLimiting ->
* 	DisplayEncoding -> 
* 	Display RGB Output
* 
*/

/** References:
* 	ACES Central, Academy of Motion Picture Arts & Sciences, https://acescentral.com/.
* 	aces-core, ACES-ASWF, https://github.com/aces-aswf/aces-core.
* 	ACESOutputTransform.ush, UnrealEngine.
*/  


// CAM constants
static const float cam_ref_luminance = 100.0;

static const float cam_L_A = 100.0;
static const float cam_Y_b = 20.0;
static const float3 cam_surround = float3(0.9, 0.59, 0.9);

static const float cam_J_scale = 100.0;
static const float cam_nl_Y_reference = 100.0;
static const float cam_nl_offset = 0.2713 * cam_nl_Y_reference;
static const float cam_nl_scale = 4.0 * cam_nl_Y_reference;

// Gamut compression
static const float smooth_cusps = 0.12;
static const float smooth_m = 0.27;
static const float cusp_mid_blend = 1.3;

static const float focus_gain_blend = 0.3;
static const float focus_adjust_gain = 0.55;
static const float focus_distance = 1.35;
static const float focus_distance_scaling = 1.75;

static const float compression_threshold = 0.75;

// Matrix for Hellwig inverse
static const float3x3 panlrcm = {
	460.0,  451.0,  288.0 ,
	460.0, -891.0, -261.0 ,
	460.0, -220.0, -6300.0
};

// TODO: InitOutputTransform

// CAM functions
float3 PostAdaptationNonLinearResponseCompressionForward(float3 RGB, float F_L) {
	float3 F_L_RGB = SafePow(F_L * abs(RGB) / 100.0, 0.42);
	return (400.0 * CopySign(float3(1.0, 1.0, 1.0), RGB) * F_L_RGB) / (27.13 + F_L_RGB);
}

float3 PostAdaptationNonLinearResponseCompressionInverse(float3 RGB, float F_L) {
	return sign(RGB) * 100.0 / F_L * SafePow((27.13 * abs(RGB)) / (400.0 - abs(RGB)), 1.0 / 0.42);
}

float PostAdaptationConeResponseCompressionForwardImpl(float Rc) {
	const float F_L_Y = pow(Rc, 0.42);
	const float Ra = (F_L_Y) / (cam_nl_offset + F_L_Y);
	return Ra;
}

float PostAdaptationConeResponseCompressionForward(float v) {
	const float Ra = PostAdaptationConeResponseCompressionForwardImpl(abs(v));
	return CopySign(Ra, v);
}

float PostAdaptationConeResponseCompressionInverseImpl(float Ra) {
	const float Ra_lim = min(Ra, 0.99);
	const float F_L_Y = (cam_nl_offset * Ra_lim) / (1.0 - Ra_lim);
	return pow(F_L_Y, 1.0 / 0.42);
}

float PostAdaptationConeResponseCompressionInverse(float v) {
	const float Rc = PostAdaptationConeResponseCompressionInverseImpl(abs(v));
	return CopySign(Rc, v);
}

float HellwigJToY(float J, float cam_surround = 0.59, float cam_L_A = 100.0, float cam_Y_b = 20.0) {
	// Viewing conditions dependent parameters (could be pre-calculated)
	float k = 1.0 / (5.0 * cam_L_A + 1.0);
	float k4 = k * k * k * k;
	float F_L = 0.2 * k4 * (5.0 * cam_L_A) + 0.1 * pow((1.0 - k4), 2.0) * pow(5.0 * cam_L_A, 1.0 / 3.0);
	float n = cam_Y_b / 100.0;
	float z = 1.48 + sqrt(n);
	float F_L_W = pow(F_L, 0.42);
	float A_w = (400.0 * F_L_W) / (27.13 + F_L_W);

	float A = A_w * pow(abs(J) / 100.0, 1.0 / (cam_surround * z));

	return sign(J) * 100.0 / F_L * pow((27.13 * A) / (400.0 - A), 1.0 / 0.42);
}

float YToHellwigJ(float Y, float cam_surround = 0.59, float cam_L_A = 100.0, float cam_Y_b = 20.0) {
	// Viewing conditions dependent parameters (could be pre-calculated)
	float k = 1.0 / (5.0 * cam_L_A + 1.0);
	float k4 = k * k * k * k;
	float F_L = 0.2 * k4 * (5.0 * cam_L_A) + 0.1 * pow((1.0 - k4), 2.0) * pow(5.0 * cam_L_A, 1.0 / 3.0);
	float n = cam_Y_b / 100.0;
	float z = 1.48 + sqrt(n);
	float F_L_W = pow(F_L, 0.42);
	float A_w = (400.0 * F_L_W) / (27.13 + F_L_W);

	float F_L_Y = pow(F_L * abs(Y) / 100.0, 0.42);

	return sign(Y) * 100.0 * pow(((400.0 * F_L_Y) / (27.13 + F_L_Y)) / A_w, cam_surround * z);
}

float3 ClampACEScg(float3 colorACEScg, float peakLuminance) {
	const float upperClampLimit = 8.0 * (128.0 + 768.0 * (log(peakLuminance / 100.0) / log(10000.0 / 100.0)));
	return clamp(colorACEScg, 0.0, upperClampLimit);
}

float AchromaticNToJ(float A, float cz) {
	return cam_J_scale * pow(A, cz);
}

float JToAchromaticN(float J) {
	return pow(J * (1.0 / cam_J_scale), ACESOutParams[0].jmh_inv_cz);
}

float AchromaticNToY(float A, float A_w_J) {
	float Ra = A_w_J * A;
	return PostAdaptationConeResponseCompressionInverseImpl(Ra) / ACESOutParams[0].jmh_F_L_n;
}

float JToY(float J, float A_w_J) {
	return AchromaticNToY(JToAchromaticN(abs(J)),  A_w_J);
}

float YToJ(float Y, float inv_A_w_J) {
	float Ra = PostAdaptationConeResponseCompressionForwardImpl(abs(Y) * ACESOutParams[0].jmh_F_L_n);
	float J = AchromaticNToJ(Ra * inv_A_w_J, ACESOutParams[0].jmh_cz);
	return CopySign(J, Y);
}

float3 JMhToAab(float3 colorJMh) {
	float h_rad = radians(colorJMh.z);

	float cos_hr;
	float sin_hr;
	sincos(h_rad, sin_hr, cos_hr);

	float A = JToAchromaticN(colorJMh.x);
	float a = colorJMh.y * cos_hr;
	float b = colorJMh.y * sin_hr;

	return float3(A, a, b);
}

float3 AabToRGB(float3 colorAab, in float3x3 aabToConeResponse, in float3x3 cam16cToRGB) {
	float3 rgb_a = mul(aabToConeResponse, colorAab);

	float3 rgb_m = float3(
		PostAdaptationConeResponseCompressionInverse(rgb_a.x),
		PostAdaptationConeResponseCompressionInverse(rgb_a.y),
		PostAdaptationConeResponseCompressionInverse(rgb_a.z)
	);

	return mul(cam16cToRGB, rgb_m);
}

float3 JMhToRGB(float3 colorJMh, in float3x3 aabToConeResponse, in float3x3 cam16cToRGB) {
	float3 colorAab = JMhToAab(colorJMh);
	return AabToRGB(colorAab, aabToConeResponse, cam16cToRGB);
}

float3 RGBToAab(float3 colorRGB, in float3x3 rgbToCAM16c, in float3x3 coneResponseToAab) {
	float3 rgb_m = mul(rgbToCAM16c, colorRGB);

	float3 rgb_a = {
		PostAdaptationConeResponseCompressionForward(rgb_m.x),
		PostAdaptationConeResponseCompressionForward(rgb_m.y),
		PostAdaptationConeResponseCompressionForward(rgb_m.z)
	};

	return mul(coneResponseToAab, rgb_a); 
}

float3 AabToJMh(float3 colorAab) {
	if (colorAab.x <= 0.0) {
		return float3(0.0, 0.0, 0.0);
	}
	float J = AchromaticNToJ(colorAab.x, ACESOutParams[0].jmh_cz);
	float M = sqrt(colorAab.y * colorAab.y + colorAab.z * colorAab.z);
	float h_rad = atan2(colorAab.z, colorAab.y);
	float h = WrapTo360(degrees(h_rad));

	return float3(J, M, h);
}

float3 RGBToJMh(float3 colorRGB, in float3x3 rgbToCAM16c, in float3x3 coneResponseToAab) {
	float3 colorAab = RGBToAab(colorRGB, rgbToCAM16c, coneResponseToAab);
	return AabToJMh(colorAab);
}

float3 ClampAP0ToAP1(float3 colorACES, float clampLowerLimit, float clampUpperLimit) {
	float3 colorAP1 = mul(AP0ToAP1Mat, colorACES);
	float3 colorClampedAP1 = clamp(colorAP1, clampLowerLimit, clampUpperLimit);
	float3 colorClampedAP0 = mul(AP1ToAP0Mat, colorClampedAP1);

	return colorClampedAP0;
}

// Chroma compression
//
// Compresses colors inside the gamut with the aim for colorfulness to have an
// appropriate rate of change from display black to display white, and from
// achromatic outward to purer colors.
float ChromaCompressionNorm(float hue) {
	float hr = radians(hue);

	float a;
	float b;
	sincos(hr, b, a);

	float cos_hr2 = a * a - b * b;
	float sin_hr2 = 2.0 * a * b;
	float cos_hr3 = 4.0 * a * a * a - 3.0 * a;
	float sin_hr3 = 3.0 * b - 4.0 * b * b * b;

	float M = 11.34072 * a 
		+ 16.46899 * cos_hr2 
		+ 7.88380 * cos_hr3 
		+ 14.66441 * b 
		+ -6.37224 * sin_hr2 
		+ 9.19364 * sin_hr3 
		+ 77.12896;

	return M * ACESOutParams[0].chromaCompressionScale;
}

// A "toe" function that remaps the given value x between 0 and limit.
// The k1 and k2 parameters change the size and shape of the toe.
// https://www.desmos.com/calculator/6vplvw14ti
float ChromaToe(float x, float limit, float k1_in, float k2_in) {
	if (x > limit) {
		return x;
	}

	const float k2 = max(k2_in, 0.001);
	const float k1 = sqrt(k1_in * k1_in + k2 * k2);
	const float k3 = (limit + k1) / (limit + k2);
	const float minus_b = k3 * x - k1;
	const float minus_c = k2 * k3 * x;

	return 0.5 * (minus_b + sqrt(minus_b * minus_b + 4.0 * minus_c));
}

float ReachMFromTable(float hue) {
	int base = HuePositionInUniformTable(hue);
	float t = hue - base;
	int lowerIndex = base + 1; // base + baseIndex
	int higherIndex = lowerIndex + 1;

	return lerp(ACESReachMTable[lowerIndex], ACESReachMTable[higherIndex], t).x;
}

// TODO: Replace odtParams
// In-gamut chroma compression
//
// Compresses colors inside the gamut with the aim for colorfulness to have an
// appropriate rate of change from display black to display white, and from
// achromatic outward to purer colors.
float3 ChromaCompressionForward(float3 colorJMh, float tonemappedJ) {
	if (colorJMh.y == 0.0) {
		return float3(tonemappedJ, colorJMh.yz);
	}

	const float nJ = tonemappedJ / ACESOutParams[0].limitJMax;
	const float snJ = max(0.0, 1.0 - nJ);
	float normM = ChromaCompressionNorm(colorJMh.z);
	float limit = pow(nJ, ACESOutParams[0].invModelGamma) * ReachMFromTable(colorJMh.z) / normM;

	float toe_limit = limit - 0.001;
	float toe_snJ_sat = snJ * ACESOutParams[0].saturation;
	float toe_sqrt_nJ_sat_thr = sqrt(nJ * nJ + ACESOutParams[0].saturationThreshold);
	float toe_nJ_compr = nJ * ACESOutParams[0].chromaCompression;

	// Forward chroma compression
	// Rescaling of M with the tonescaled J to get the M to the same range as
	// J after the tonescale.  The rescaling uses the Hellwig2022 model gamma to
	// keep the M/J ratio correct (keeping the chromaticities constant).
	float compressedM = colorJMh.y * pow(tonemappedJ / colorJMh.x, ACESOutParams[0].invModelGamma); 

	// Normalize M with the rendering space cusp M
	compressedM /= normM;

	// Expand the colorfulness by running the toe function in reverse.  The goal is to
	// expand less saturated colors less and more saturated colors more.  The expansion
	// increases saturation in the shadows and mid-tones but not in the highlights.
	// The 0.001 offset starts the expansions slightly above zero.  The sat_thr makes
	// the toe less aggressive near black to reduce the expansion of noise.
	compressedM = limit - ChromaToe(limit - compressedM, toe_limit, toe_snJ_sat, toe_sqrt_nJ_sat_thr);

	// Compress the colorfulness.  The goal is to compress less saturated colors more and
	// more saturated colors less, especially in the highlights.  This step creates the
	// saturation roll-off in the highlights, but attemps to preserve pure colors.  This
	// mostly affects highlights and mid-tones, and does not compress shadows.
	compressedM = ChromaToe(compressedM, limit, toe_nJ_compr, snJ);

	// Denormalize
	compressedM *= normM;

	return float3(tonemappedJ, compressedM, colorJMh.z);
}

float2 CuspFromTable(float hue) {
	float3 lower = 0.0f;
	float3 higher = 0.0f;

	int lowIndex = 0;
	int highIndex = 1 + tableSize; // baseIndex + tableSize
	int i = HuePositionInUniformTable(hue) + 1;

	// Binrary search: In worst case would cause log2(360) ~= 9 times Load().
	while (lowIndex + 1 < highIndex) {
		if (hue > ACESGamutCuspsTable[i].z) {
			lowIndex = i;
		}
		else {
			highIndex = i;
		}
		i = MidPoint(lowIndex, highIndex);
	}
	lower = ACESGamutCuspsTable[highIndex - 1].xyz;
	higher = ACESGamutCuspsTable[highIndex].xyz;

	float t = (hue - lower.z) / (higher.z - lower.z);
	float cuspJ = lerp(lower.x, higher.x, t);
	float cuspM = lerp(lower.y, higher.y, t);

	return float2(cuspJ, cuspM);
}

uint LookupHueInterval(float hue) {
	// Search the given table for the interval containing the desired hue
	// Returns the upper index of the interval

	// We can narrow the search range based on the hues being almost uniform
	uint i = 1 + HuePositionInUniformTable(hue, paddedTableSize);
	uint lowerIndex = max(1, i + ACESOutParams[0].hueLinearitySearchRange.x);
	uint higherIndex = min(1 + tableSize, i + ACESOutParams[0].hueLinearitySearchRange.y);

	while (lowerIndex + 1 < higherIndex) {
		if (hue > ACESGamutCuspsTable[i].z) {
			lowerIndex = i;
		}
		else {
			higherIndex = i;
		}
		i = MidPoint(lowerIndex, higherIndex);
	}

	higherIndex = max(1, higherIndex);
	return higherIndex;
}

float ComputeFocusJ(float cuspJ) {
	return lerp(cuspJ, ACESOutParams[0].midJ, min(1.0, cusp_mid_blend - (cuspJ / ACESOutParams[0].limitJMax)));
}

float GetFocusGain(float J, float analyticalThreshold) {
	float gain = ACESOutParams[0].limitJMax * ACESOutParams[0].focusDistance;
	if (J > analyticalThreshold) {
		// Approximate inverse required above threshold due to the introduction of J in the calculation
		float gainAdjustment = log10((ACESOutParams[0].limitJMax - analyticalThreshold) / max(0.0001, ACESOutParams[0].limitJMax - J));
		gainAdjustment = gainAdjustment * gainAdjustment + 1.0;
		gain *= gainAdjustment;
	}
	return gain;
}

float SolveIntersectJ(float colorJ, float colorM, float focusJ, float slopeGain) {
	const float scaledM = colorM / slopeGain;
	const float a = scaledM / focusJ;

	if (colorJ < focusJ) {
		const float b = 1.0 - scaledM;
		const float c = -colorJ;
		const float det = b * b - 4.0 * a * c;
		const float root = sqrt(det);
		return -2.0 * c / (b + root);
	}
	else {
		const float b = -(1.0 + scaledM + ACESOutParams[0].limitJMax * a);
		const float c = ACESOutParams[0].limitJMax * scaledM + colorJ;
		const float det = b * b - 4.0 * a * c;
		const float root = sqrt(det);
		return -2.0 * c / (b - root);
	}
}

float ComputeCompressionVectorSlope(float intersectJ, float focusJ, float slopeGain) {
	float directionScalar = 0.0;
	if (intersectJ < focusJ) {
		directionScalar = intersectJ;
	}
	else {
		directionScalar = ACESOutParams[0].limitJMax - intersectJ;
	}
	return directionScalar * (intersectJ - focusJ) / (focusJ * slopeGain);
}

float SmoothMinimumScaled(float a, float b, float scaleReference) {
	const float scaledS = smooth_cusps * scaleReference;
	const float h = max(scaledS - abs(a - b), 0.0) / scaledS;
	return min(a, b) - h * h * h * scaledS * (1.0 / 6.0);
}

float EstimateLineAndBoundaryIntersectionM(
	float axisIntersectJ, float slope, float invGamma, float maxJ, float maxM, float intersectionReferenceJ
) {
	// Line defined by	 J = slope * x + J_axis_intersect
	// Boundary defined by J = J_max * (x / M_max) ^ (1/inv_gamma)
	// Approximate as we do not want to iteratively solve intersection of a
	// straight line and an exponential

	// We calculate a shifted intersection from the original intersection using
	// the inverse of the exponential and the provided reference
	const float normalisedJ = axisIntersectJ / intersectionReferenceJ;
	const float shiftedIntersection = intersectionReferenceJ * pow(normalisedJ, invGamma);

	// Now we find the M intersection of two lines
	// line from origin to J,M Max	   l1(x) = J/M * x
	// line from J Intersect' with slope l2(x) = slope * x + Intersect'

	// return shifted_intersection / ((J_max / M_max) - slope);
	return shiftedIntersection * maxM / (maxJ - slope * maxM);
}

float FindGamutBoundaryIntersection(
	float2 cuspJM, float invGammaTop, float intersectSourceJ, float slope, float intersectCuspJ
) {
	const float boundaryLowerM = EstimateLineAndBoundaryIntersectionM(
		intersectSourceJ, slope, ACESOutParams[0].invLowerHullGamma, cuspJM.x, cuspJM.y, intersectCuspJ
	);

	// The upper hull is flipped and thus 'zeroed' at J_max
	// Also note we negate the slope
	const float intersectCuspFJ = ACESOutParams[0].limitJMax - intersectCuspJ;
	const float intersectSourceFJ = ACESOutParams[0].limitJMax - intersectSourceJ;
	const float fJMCuspJ = ACESOutParams[0].limitJMax - cuspJM.x;
	const float boundaryUpperM = EstimateLineAndBoundaryIntersectionM(
		intersectSourceFJ, -slope, invGammaTop, fJMCuspJ, cuspJM.y, intersectCuspFJ
	);

	// Smooth minimum between the two calculated values for the M component
	float boundaryM = SmoothMinimumScaled(boundaryLowerM, boundaryUpperM, cuspJM.y);
	return boundaryM;
}


float ReinhardRemap(float scale, float nd) {
	return scale * nd / (1.0 + nd);
}

float RemapM(float M, float gamutBoundaryM, float reachBoundaryM) {
	const float boundaryRatio = gamutBoundaryM / reachBoundaryM;
	const float proportion = max(boundaryRatio, compression_threshold);
	const float threshold = proportion * gamutBoundaryM;

	if (M <= threshold || proportion >= 1.0f)
		return M;

	// Translate to place threshold at zero
	const float mOffset = M - threshold;
	const float gamutOffset = gamutBoundaryM - threshold;
	const float reachOffset = reachBoundaryM - threshold;

	const float scale = reachOffset / ((reachOffset / gamutOffset) - 1.0);
	const float nd = mOffset / scale;

	// Shift result back to absolute
	return threshold + ReinhardRemap(scale, nd);
}

float3 GamutCompressionForward(float3 colorJMh) {
	float limitJMax = ACESOutParams[0].limitJMax;

	if (colorJMh.x <= 0.0) {
		return float3(0.0, 0.0, colorJMh.z);
	}

	if (colorJMh.y < 0.0 || colorJMh.x > limitJMax) {
		return float3(colorJMh.x, 0.0, colorJMh.z);
	}

	// HueDependentGamutParams
	const int higherIndex = LookupHueInterval(colorJMh.z);
	const float t = colorJMh.z - ACESGamutCuspsTable[higherIndex - 1].z;

	float2 cuspJM = CuspFromTable(colorJMh.z);
	float invGammaTop = lerp(
		ACESUpperHullGammaTable[higherIndex - 1], ACESUpperHullGammaTable[higherIndex], t
	).x;
	float focusJ = ComputeFocusJ(cuspJM.x);
	float analyticalThreshold = lerp(cuspJM.x, limitJMax, focus_gain_blend);

	// CompressGamut(colorJMh, colorJMh.x);
	const float slopeGain = GetFocusGain(colorJMh.x, analyticalThreshold);
	const float intersectSourceJ = SolveIntersectJ(colorJMh.x, colorJMh.y, focusJ, slopeGain);
	const float gamutSlope = ComputeCompressionVectorSlope(intersectSourceJ, focusJ, slopeGain);
	const float intersectCuspJ = SolveIntersectJ(cuspJM.x, cuspJM.y, focusJ, slopeGain);

	const float gamutBoundaryM = FindGamutBoundaryIntersection(
		cuspJM, invGammaTop, intersectSourceJ, gamutSlope, intersectCuspJ
	);

	if (gamutBoundaryM <= 0.0) {
		return float3(colorJMh.x, 0.0, colorJMh.z);
	}

	float reachMaxM = ReachMFromTable(colorJMh.z);

	const float reachBoundaryM = EstimateLineAndBoundaryIntersectionM(
		intersectSourceJ, gamutSlope, ACESOutParams[0].invModelGamma, limitJMax, reachMaxM, limitJMax
	);

	const float remappedM = RemapM(colorJMh.y, gamutBoundaryM, reachBoundaryM);

	float3 JMhcompressed = {
		intersectSourceJ + remappedM * gamutSlope,
		remappedM,
		colorJMh.z
	};
	return JMhcompressed;
}

/**
* @brief: JMh procedures. Tone scale -> Chroma compression -> Gamut Compression 
* @param acesACES: Color which was defined in ACES2065-1 (AP0).  
* @return: CIE XYZ color space, Display Encodings put in other functions for extensibility.
*/
float3 OutputTransformFoward(float3 colorACES) {
	// Rendering tranform: ACES2065-1 (AP0) -> ACEScg -> JMh
	// JMh: lightness, colorfulness and hue.
	float3 colorClampedACES = ClampAP0ToAP1(colorACES, 0.0, ACESOutParams[0].tonescale_forwardLimit);
	float3 colorJMh = RGBToJMh(
		colorClampedACES, 
		Float4x3ToFloat3x3(ACESOutParams[0].inputJMh_rgbToCAM16c), 
		Float4x3ToFloat3x3(ACESOutParams[0].inputJMh_coneResponseToAab)
	);

	// Tonemapping: Tonescale -> ChromaCompression
	float colorY = JToY(colorJMh.x, ACESOutParams[0].inputJMh_A_w_J) / cam_ref_luminance; /* CIE XYZ ColorSpace*/
	float tonescaledLuminance = TonescaleForward(colorY); /* CIE XYZ ColorSpace*/
	float tonescaledJ = YToJ(tonescaledLuminance, ACESOutParams[0].inputJMh_inv_A_w_J);
	float3 tonemappedJMh = ChromaCompressionForward(colorJMh, tonescaledJ);
	// float3 tonemappedJMh = float3(tonescaledJ, chromaCompressedM, colorJMh.z);

	// GamutCompression
	float3 compressedJMh = GamutCompressionForward(tonemappedJMh);

	// Return CIE XYZ Color
	return JMhToRGB(
		compressedJMh, 
		Float4x3ToFloat3x3(ACESOutParams[0].limitJMh_aabToConeResponse), 
		Float4x3ToFloat3x3(ACESOutParams[0].limitJMh_cam16cToRGB)
	);

	// TEST
	// return JMhToRGB(
	// 	float3(tonescaledJ, colorJMh.yz), 
	// 	Float4x3ToFloat3x3(ACESOutParams[0].limitJMh_aabToConeResponse), 
	// 	Float4x3ToFloat3x3(ACESOutParams[0].limitJMh_cam16cToRGB)
	// );
}

// TODO: Display encoding by Vulkan API due to the GUI issue.
// float3 DisplayEncoding(float3 colorRGB) {...}

// TODO: .........
/** 
* @param sceneReferredLinearColor: source color which in the working space.
* @return Target device color space value.
*/
// float3 ACESOutputTransform(float3 sceneReferredLinearColor) {
// 	OutputTransformParams odtParams = InitOutputTransformParams(...);
// 	float3 colorACES = mul(workingColorSpaceToACES, sceneReferredLinearColor, odtParams);
// 	float3 colorXYZ = OutputTransformACESToXYZ(colorACES, odtParams);
// 	if (scaleWhite) {
// 		colorXYZ = ApplyWhiteScaleXYZ(colorXYZ, odtParams);
// 	}
// 	// EOTF by the Vulkan API due to the GUI issue.
// 	// return DisplayEncoding(colorXYZ);
// 	return mul(XYZToOutputDeviceRGB, colorXYZ); //ColorXYZToOutputDeviceRGB(colorXYZ);
// }

#endif