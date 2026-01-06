#include "ACESUtilities.h"

#include <cmath>
#include <stdexcept>
#include <algorithm>

aces::OutputTransformParamsWrapper& aces::OutputTransformContext() {
	static OutputTransformParamsWrapper wrapper{};
	return wrapper;
}

float aces::WrapTo360(float x) {
	float y = fmod(x, 360.0f);
	if (y < 0.0f) {
		y += 360.0f;
	}
	return y;
}

aces::float3x3 aces::ScaleMatrixDiagonal(const float3x3& mat, const float3& scale) noexcept {
	float3x3 result = mat;
	result[0][0] = mat[0][0] * scale[0];
	result[1][1] = mat[1][1] * scale[1];
	result[2][2] = mat[2][2] * scale[2];
	return result;
}

float aces::BaseHueForPosition(uint32_t index) noexcept {
	return index * (hueLimit / tableSize);
}

uint32_t aces::HuePositionInUniformTable(float hue, uint32_t size) noexcept {
	const float wrappedHue = WrapTo360(hue);
	return wrappedHue / hueLimit * size;
}

float aces::PostAdaptationConeResponseCompressionForwardImpl(float Rc) noexcept {
	const float F_L_Y = pow(Rc, 0.42f);
	return (F_L_Y) / (cam_nl_offset + F_L_Y);
}

float aces::PostAdaptationConeResponseCompressionForward(float v) noexcept {
	const float Ra = PostAdaptationConeResponseCompressionForwardImpl(abs(v));
	return copysign(Ra, v);
}

float aces::PostAdaptationConeResponseCompressionInverseImpl(float Ra) noexcept {
	const float Ra_lim = std::min(Ra, 0.99f);
	const float F_L_Y = (cam_nl_offset * Ra_lim) / (1.0f - Ra_lim);
	return pow(F_L_Y, 1.0f / 0.42f);
}

float aces::PostAdaptationConeResponseCompressionInverse(float v) noexcept {
	const float Rc = PostAdaptationConeResponseCompressionInverseImpl(abs(v));
	return copysign(Rc, v);
}

float aces::JToAchromaticN(float J, float inv_cz) noexcept {
	return pow(J * (1.0f / cam_J_scale), inv_cz);
}

float aces::AchromaticNToJ(float A, float cz) noexcept {
	return cam_J_scale * pow(A, cz);
}

float aces::YToJ(float Y, const JMhParams& params) noexcept {
	float Ra = PostAdaptationConeResponseCompressionForwardImpl(abs(Y) * params.F_L_n);
	float J = AchromaticNToJ(Ra * params.inv_A_w_J, params.cz);
	return copysign(J, Y);
}

aces::float3 aces::AabToJMh(const float3& colorAab, const JMhParams& jmhParams) noexcept {
	if (colorAab.x <= 0.0f) {
		return float3{ 0.0f, 0.0f, 0.0f };
	}
	float J = AchromaticNToJ(colorAab.x, jmhParams.cz);
	float M = sqrt(colorAab.y * colorAab.y + colorAab.z * colorAab.z);
	float h_rad = atan2(colorAab.z, colorAab.y);
	float h = WrapTo360(Degrees(h_rad));

	return float3{ J, M, h };
}

aces::float3 aces::JMhToAab(const float3& colorJMh, const JMhParams& jmhParams) noexcept {
	float h_rad = Radians(colorJMh.z);
	float cos_hr = cos(h_rad);
	float sin_hr = sin(h_rad);

	float A = JToAchromaticN(colorJMh.x, jmhParams.inv_cz);
	float a = colorJMh.y * cos_hr;
	float b = colorJMh.y * sin_hr;

	return float3(A, a, b);
}

aces::float3 aces::RGBToAab(const float3& colorRGB, const JMhParams& jmhParams) noexcept {
	float3 rgb_m = jmhParams.rgbToCAM16c * colorRGB;
	float3 rgb_a = {
		PostAdaptationConeResponseCompressionForward(rgb_m.x),
		PostAdaptationConeResponseCompressionForward(rgb_m.y),
		PostAdaptationConeResponseCompressionForward(rgb_m.z)
	};

	float3 Aab = jmhParams.coneResponseToAab * rgb_a;
	return Aab;
}

aces::float3 aces::AabToRGB(const float3& colorAab, const JMhParams& jmhParams) noexcept {
	float3 rgb_a = jmhParams.aabToConeResponse * colorAab;

	float3 rgb_m = float3(
		PostAdaptationConeResponseCompressionInverse(rgb_a.x),
		PostAdaptationConeResponseCompressionInverse(rgb_a.y),
		PostAdaptationConeResponseCompressionInverse(rgb_a.z)
	);

	return jmhParams.cam16cToRGB * rgb_m;
}

aces::float3 aces::RGBToJMh(const float3& colorRGB, const JMhParams& jmhParams) noexcept {
	float3 colorAab = RGBToAab(colorRGB, jmhParams);
	return AabToJMh(colorAab, jmhParams);
}

aces::float3 aces::JMhToRGB(const float3& colorJMh, const JMhParams& jmhParams) noexcept {
	float3 colorAab = JMhToAab(colorJMh, jmhParams);
	return AabToRGB(colorAab, jmhParams);
}

aces::float3 aces::GenerateUnitCubeCuspCorners(uint32_t cornerIndex) {
	// Generation order R, Y, G, C, B, M to ensure hues rotate in correct order
	return float3 { 
		((cornerIndex + 1) % cuspCornerCount) < 3 ? 1.0f : 0.0f, 
		((cornerIndex + 5) % cuspCornerCount) < 3 ? 1.0f : 0.0f, 
		((cornerIndex + 3) % cuspCornerCount) < 3 ? 1.0f : 0.0f
	};
}

void aces::BuildLimitingCuspCorners(CuspCorners& limitingRGBCorners, CuspCorners& limitingJMhCorners, const JMhParams& limitParams, float peakLuminance) {
	// We calculate the RGB and JMh values for the limiting gamut cusp corners
	std::array<float3, cuspCornerCount> tempRGBCorners{};
	std::array<float3, cuspCornerCount> tempJMhCorners{};

	int32_t min_index = 0;
	for (int32_t i = 0; i < cuspCornerCount; ++i) {
		tempRGBCorners[i] = GenerateUnitCubeCuspCorners(i) * (peakLuminance / cam_ref_luminance);
		tempJMhCorners[i] = RGBToJMh(tempRGBCorners[i], limitParams);
		// Find the minimum hue index
		if (tempJMhCorners[i].z < tempJMhCorners[min_index].z)
			min_index = 1;
	}
	// Rotate entries placing lowest at [0]
	for (size_t i = 0; i < cuspCornerCount; ++i) {
		limitingRGBCorners[i + 1] = tempRGBCorners[(i + min_index) % cuspCornerCount];
		limitingJMhCorners[i + 1] = tempJMhCorners[(i + min_index) % cuspCornerCount];
	}

	// Copy end elements to create a cycle
	constexpr int32_t lastCornerIndex = paddedCuspCornerCount - 1;
	limitingRGBCorners[0] = limitingRGBCorners[cuspCornerCount];
	limitingRGBCorners[lastCornerIndex] = limitingRGBCorners[1];
	limitingJMhCorners[0] = limitingJMhCorners[cuspCornerCount];
	limitingJMhCorners[lastCornerIndex] = limitingJMhCorners[1];

	// Wrap the hues, to maintain monotonicity these entries will fall outside [0.0, hue_limit)
	limitingJMhCorners[0].z -= hueLimit;
	limitingJMhCorners[lastCornerIndex].z += hueLimit;
}

aces::CuspCorners aces::FindReachCorners(const JMhParams& reachParams, float limitJMax, float toneForwardLimit) {
	// We need to find the value of JMh that corresponds to limitJ for each
	// corner This is done by scaling the unit corners converting to JMh until
	// the J value is near the limitJ
	// As an optimisation we use the equivalent Achromatic value to search for
	// the J value and avoid the non-linear transform during the search.
	// Strictly speaking we should only need to find the R, G and  B "corners"
	// as the reach is unbounded and as such does not form a cube, but is formed
	// by the transformed 3 lower planes of the cube and the plane at J = limitJ
	constexpr float reachCuspTolerance = 1e-3f;

	std::array<float3, cuspCornerCount> tempCorners{};
	CuspCorners jmhCorners{};

	float limitA = JToAchromaticN(limitJMax, reachParams.inv_cz);

	int32_t minIndex = 0;
	for (int32_t i = 0; i < cuspCornerCount; ++i) {
		float3 rgbVector = GenerateUnitCubeCuspCorners(i);

		float lower = 0.0f;
		float upper = toneForwardLimit;

		while ((upper - lower) > reachCuspTolerance) {
			float test = (lower + upper) / 2.0f;
			float3 testCorner = rgbVector * test;
			float A = RGBToAab(testCorner, reachParams).x;
			if (A < limitA) {
				lower = test;
			}
			else {
				upper = test;
			}
		}

		tempCorners[i] = RGBToJMh(rgbVector * upper, reachParams);

		// finding the min hue position
		if (tempCorners[i].z < tempCorners[minIndex].z)
			minIndex = i;
	}

	// Rotate entries placing lowest at [0]
	for (size_t i = 0; i < cuspCornerCount; ++i) {
		jmhCorners[i + 1] = tempCorners[(i + minIndex) % cuspCornerCount];
	}

	// Copy end elements to create a cycle
	constexpr int32_t lastCornerIndex = paddedCuspCornerCount - 1;
	jmhCorners[0] = jmhCorners[cuspCornerCount];
	jmhCorners[lastCornerIndex] = jmhCorners[1];

	// Wrap the hues, to maintain monotonicity these entries will fall outside [0.0, hue_limit)
	jmhCorners[0].z -= hueLimit;
	jmhCorners[lastCornerIndex].z += hueLimit;

	return jmhCorners;
}


aces::SortedCornerHues aces::ExtractSortedCubeHues(const CuspCorners& reachJMhCorners, const CuspCorners& limitJMhCorners) {
	SortedCornerHues sortedCornerHues{};

	// Basic merge of 2 sorted arrays, extracting the unique hues.
	// Return the count of the unique hues
	int32_t index = 0;
	int32_t reachIndex = 1;
	int32_t limitIndex = 1;
	while ((reachIndex < cuspCornerCount + 1) || (limitIndex < cuspCornerCount + 1)) {
		float reachHue = reachJMhCorners[reachIndex].z;
		float limitHue = limitJMhCorners[limitIndex].z;
		if (reachHue == limitHue) {
			sortedCornerHues[index] = reachHue;
			++reachIndex;
			++limitIndex; // When equal consume both
		}
		else {
			if (reachHue < limitHue) {
				sortedCornerHues[index] = reachHue;
				++reachIndex;
			}
			else {
				sortedCornerHues[index] = limitHue;
				++limitIndex;
			}
		}
		++index;
	}
	return sortedCornerHues;
}

aces::float2 aces::FindDisplayCuspForHue(float hue, const CuspCorners& rgbCorners, const CuspCorners& jmhCorners, const JMhParams& jmhParams) {
	// This works by finding the required line segment between two of the XYZ
	// cusp corners, then binary searching along the line calculating the JMh of
	// points along the line till we find the required value. All values on the
	// line segments are valid cusp locations.
	int32_t upperCorner = 1;
	for (int32_t i = upperCorner; i < paddedCuspCornerCount; ++i) {
		if (jmhCorners[i].z > hue) {
			upperCorner = i;
			break;
		}
		// TODO: Should feelback some warning here.
	}
	int32_t lowerCorner = upperCorner - 1;

	// hue should now be within [lower_corner, upper_corner), handle exact match
	if (jmhCorners[lowerCorner].z == hue) {
		float3 jmhCorner = jmhCorners[lowerCorner];
		return { jmhCorner.x, jmhCorner.y };
	}

	// search by lerping between RGB corners for the hue
	float3 cuspLower = rgbCorners[lowerCorner];
	float3 cuspUpper = rgbCorners[upperCorner];

	float sampleT = 0.0f;
	float lowerT = 0.0f;
	// nothing happen
	//if (upperCorner == previous[0])
	//	lower_t = previous[1];
	float upperT = 1.0f;

	float3 sample{};
	float3 colorJMh{};

	// There is an edge case where we need to search towards the range when
	// across the [0.0, hue_limit] boundary each edge needs the directions
	// swapped. This is handled by comparing against the appropriate corner to
	// make sure we are still in the expected range between the lower and upper
	// corner hue limits
	constexpr float displayCuspTolerance = 1e-7f;
	while ((upperT - lowerT) > displayCuspTolerance) {
		sampleT = MidPoint(lowerT, upperT);
		sample = Lerp(cuspLower, cuspUpper, sampleT);
		colorJMh = RGBToJMh(sample, jmhParams);
		if (colorJMh.z < jmhCorners[lowerCorner].z) {
			upperT = sampleT;
		}
		else if (colorJMh.z >= jmhCorners[upperCorner].z) {
			lowerT = sampleT;
		}
		else if (colorJMh.z > hue) {
			upperT = sampleT;
		}
		else {
			lowerT = sampleT;
		}
	}

	// Use the midpoint of the final interval for the actual samples
	sampleT = MidPoint(lowerT, upperT);
	sample = Lerp(cuspLower, cuspUpper, sampleT);
	colorJMh = RGBToJMh(sample, jmhParams);

	// Why here commit out, WTF the aces want to do?
	// previous[0] = upper_corner;
	// previous[1] = sample_t;

	return { colorJMh.x, colorJMh.y };
}

void aces::BuildHueSampleInterval(HueTable& hueTable, int32_t samples, float lower, float upper, int32_t baseIndex) {
	float delta = (upper - lower) / samples;
	for (size_t i = 0; i < samples; ++i) {
		hueTable[i + baseIndex] = lower + i * delta;
	}
}

void aces::BuildHueTable(HueTable& hueTable, const SortedCornerHues& sortedHues) {
	// @deprecated: Can not do that, hue is not uniform.
	//constexpr float mappingRatio = static_cast<float>(SortedCornerHues{}.size()) / HueTable{}.size();
	//for (size_t index = 0; index < hueTable.size(); ++index) {
	//	float t = mappingRatio * index;
	//	int32_t aIndex = static_cast<int32_t>(floor(t));
	//	int32_t bIndex = static_cast<int32_t>(ceil(t));
	//	if (bIndex == aIndex && bIndex < (HueTable{}.size() - 1)) {
	//		++bIndex;
	//	}
	//	// normalize t
	//	t -= aIndex;
	//	hueTable[index] = (1.0f - t) * sortedHues[aIndex] + t * sortedHues[bIndex];
	//}
	//return hueTablePtr;

	float idealSpacing = tableSize / hueLimit;
	std::array<int32_t, maxSortedCornerCount + 2> samplesCount{};
	int32_t lastIndex = -1;
	int32_t minIndex = sortedHues[0] == 0.0 ? 0 : 1;

	for (size_t hueIndex = 0; hueIndex < maxSortedCornerCount; ++hueIndex) {
		int32_t nominalIndex = std::clamp(
			// i = 0, 120, 240, ....
			static_cast<int32_t>(round(sortedHues[hueIndex] * idealSpacing)), 
			minIndex, 
			static_cast<int32_t>(tableSize) - 1
		);

		if (lastIndex == nominalIndex) {
			// Last two hues should sample at same index, need to adjust them
			// Adjust previous sample down if we can
			// For index continuity: 1 ... 2 ... 3?
			if (hueIndex > 1 && samplesCount[hueIndex - 2] != (samplesCount[hueIndex - 1] - 1)) {
				samplesCount[hueIndex - 1] = samplesCount[hueIndex - 1] - 1;
			}
			else {
				nominalIndex = nominalIndex + 1;
			}
		}
		samplesCount[hueIndex] = std::min(nominalIndex, static_cast<int32_t>(tableSize) - 1);
		minIndex = nominalIndex;
		lastIndex = minIndex;
	}

	int32_t totalSamples = 0;

	// Special cases for ends
	BuildHueSampleInterval(hueTable, samplesCount[0], 0.0, sortedHues[0], totalSamples + 1);
	totalSamples = totalSamples + samplesCount[0];

	for (size_t i = 1; i < maxSortedCornerCount; ++i) {
		int32_t samples = samplesCount[i] - samplesCount[i - 1];
		BuildHueSampleInterval(hueTable, samples, sortedHues[i - 1], sortedHues[i], totalSamples + 1);
		totalSamples = totalSamples + samples;
	}
	BuildHueSampleInterval(hueTable, tableSize - totalSamples, sortedHues[static_cast<size_t>(maxSortedCornerCount) - 1], hueLimit, totalSamples + 1);

	// Cyclic Padding
	hueTable[0] = hueTable[tableSize] - hueLimit;
	hueTable[static_cast<size_t>(paddedTableSize) - 1] = hueTable[1] + hueLimit;
}

void aces::BuildCuspsTable(
	GamutCuspsTable& cuspsTable, 
	const HueTable& hueTable, 
	const CuspCorners& rgbCorners, 
	const CuspCorners& jmhCorners, 
	const JMhParams& jmhParams
) {
	//float2 previous = { 0.0f, 0.0f };

	for (int32_t i = 1; i < paddedTableSize; ++i) {
		float hue = hueTable[i];
		float2 colorJM = FindDisplayCuspForHue(hue, rgbCorners, jmhCorners, jmhParams);
		cuspsTable[i] = { colorJM.x, colorJM.y * (1.0f + smooth_m * smooth_cusps), hue, 0.0f };
	}

	// TODO: i 0 and 360 seems not right
	// Copy last nominal entry to start
	cuspsTable[0] = cuspsTable[tableSize];
	cuspsTable[0].z = hueTable[0];

	// Copy first nominal entry to end
	constexpr int32_t lastIndex = paddedTableSize - 1;
	cuspsTable[lastIndex] = cuspsTable[1];
	cuspsTable[lastIndex].z = hueTable[lastIndex];
}

float aces::ComputeFocusJ(float cuspJ, float midJ, float limitJMax) noexcept {
	return Lerp(cuspJ, midJ, std::min(1.0f, cusp_mid_blend - (cuspJ / limitJMax)));
}

float aces::GetFocusGain(float colorJ, float analyticalThreshold, float limitJMax, float focusDistance) noexcept {
	float gain = limitJMax * focusDistance;

	if (colorJ > analyticalThreshold) {
		// Approximate inverse required above threshold due to the introduction of J in the calculation
		float gainAdjustment = log10((limitJMax - analyticalThreshold) / std::max(0.0001f, limitJMax - colorJ));
		gainAdjustment = gainAdjustment * gainAdjustment + 1.0f;
		gain *= gainAdjustment;
	}

	return gain;
}

float aces::SolveIntersectJ(float colorJ, float colorM, float focusJ, float maxJ, float slopeGain)  noexcept {
	const float scaledM = colorM / slopeGain;
	const float a = scaledM / focusJ;

	if (colorJ < focusJ) {
		const float b = 1.0f - scaledM;
		const float c = -colorJ;
		const float det = b * b - 4.0f * a * c;
		const float root = sqrt(det);
		return -2.0f * c / (b + root);
	}
	else {
		const float b = -(1.0f + scaledM + maxJ * a);
		const float c = maxJ * scaledM + colorJ;
		const float det = b * b - 4.0f * a * c;
		const float root = sqrt(det);
		return -2.0f * c / (b - root);
	}
}

float aces::ComputeCompressionVectorSlope(float intersectJ, float focusJ, float limitJMax, float slopeGain) noexcept {
	float directionScalar = 0.0f;
	if (intersectJ < focusJ) {
		directionScalar = intersectJ;
	}
	else {
		directionScalar = limitJMax - intersectJ;
	}
	return directionScalar * (intersectJ - focusJ) / (focusJ * slopeGain);
}

float aces::SmoothMinimumScaled(float a, float b, float scaleReference) noexcept {
	const float scaledS = smooth_cusps * scaleReference;
	const float h = std::max(scaledS - abs(a - b), 0.0f) / scaledS;
	return std::min(a, b) - h * h * h * scaledS * (1.0f / 6.0f);
}

float aces::EstimateLineAndBoundaryIntersectionM(float axisIntersectJ, float slope, float invGamma, float maxJ, float maxM, float intersectionReferenceJ) noexcept {
	// Line defined by     J = slope * x + J_axis_intersect
	// Boundary defined by J = J_max * (x / M_max) ^ (1/inv_gamma)
	// Approximate as we do not want to iteratively solve intersection of a
	// straight line and an exponential

	// We calculate a shifted intersection from the original intersection using
	// the inverse of the exponential and the provided reference
	const float normalisedJ = axisIntersectJ / intersectionReferenceJ;
	const float shiftedIntersection = intersectionReferenceJ * pow(normalisedJ, invGamma);

	// Now we find the M intersection of two lines
	// line from origin to J,M Max       l1(x) = J/M * x
	// line from J Intersect' with slope l2(x) = slope * x + Intersect'

	// return shifted_intersection / ((J_max / M_max) - slope);
	return shiftedIntersection * maxM / (maxJ - slope * maxM);
}

float aces::FindGamutBoundaryIntersection(
	float2 cuspJM, 
	float maxJ, 
	float invGammaTop, 
	float invGammaBottom, 
	float intersectSourceJ, 
	float slope, 
	float intersectCuspJ
) noexcept {
	const float boundaryLowerM = EstimateLineAndBoundaryIntersectionM(
		intersectSourceJ, slope, invGammaBottom, cuspJM.x, cuspJM.y, intersectCuspJ
	);

	// The upper hull is flipped and thus 'zeroed' at J_max
	// Also note we negate the slope
	const float intersectCuspFJ = maxJ - intersectCuspJ;
	const float intersectSourceFJ = maxJ - intersectSourceJ;
	const float fJMCuspJ = maxJ - cuspJM.x;
	const float boundaryUpperM = EstimateLineAndBoundaryIntersectionM(
		intersectSourceFJ, -slope, invGammaTop, fJMCuspJ, cuspJM.y, intersectCuspFJ
	);

	// Smooth minimum between the two calculated values for the M component
	float boundaryM = SmoothMinimumScaled(boundaryLowerM, boundaryUpperM, cuspJM.y);
	return boundaryM;
}

bool aces::EvaluateGammaFit(
	float2 cuspJM, 
	float invTopGamma, 
	float peakLuminance, 
	float limitJMax, 
	float invLowerHullGamma, 
	const std::array<float3, upperHullGammaTestCount>& testJMhs, 
	const std::array<float, upperHullGammaTestCount>& intersectSourceJs, 
	const std::array<float, upperHullGammaTestCount>& slopes, 
	const std::array<float, upperHullGammaTestCount>& intersectCuspJs, 
	const JMhParams& limitParams
) {
	float luminanceLimit = peakLuminance / cam_ref_luminance;

	for (int32_t testIndex = 0; testIndex < upperHullGammaTestCount; ++testIndex) {
		// Compute gamut boundary intersection
		float approxLimitM = FindGamutBoundaryIntersection(
			cuspJM, 
			limitJMax, 
			invTopGamma, 
			invLowerHullGamma,
			intersectSourceJs[testIndex],
			slopes[testIndex],
			intersectCuspJs[testIndex]
		);
		float approxLimitJ = intersectSourceJs[testIndex] + slopes[testIndex] * approxLimitM;

		// Store JMh values
		float3 approximateJMh = { approxLimitJ, approxLimitM, testJMhs[testIndex].z };

		// Convert to RGB
		float3 newLimitRGB = JMhToRGB(approximateJMh, limitParams);

		// Check if any values exceed the luminance limit. If so, we are outside of the top gamut shell.
		bool outsideHill = (newLimitRGB.x > luminanceLimit) || (newLimitRGB.y > luminanceLimit) || (newLimitRGB.z > luminanceLimit);
		if (!outsideHill) {
			return false;
		}
	}

	return true;
}

void aces::GenerateGammaTestData(
	const float2 cuspJM, 
	const float hue, 
	const float limitJMax, 
	const float midJ, 
	const float focusDistance, 
	std::array<float3, upperHullGammaTestCount>& testJMhs, 
	std::array<float, upperHullGammaTestCount>& intersectSourceJs, 
	std::array<float, upperHullGammaTestCount>& slopes, 
	std::array<float, upperHullGammaTestCount>& intersectCuspJs
) {
	float analyticalThreshold = Lerp(cuspJM.x, limitJMax, focus_gain_blend);
	float focusJ = ComputeFocusJ(cuspJM.x, midJ, limitJMax);

	constexpr std::array<float, upperHullGammaTestCount> testPositions = { 0.01f, 0.1f, 0.5f, 0.8f, 0.99f };

	for (int32_t testIndex = 0; testIndex < upperHullGammaTestCount; ++testIndex) {
		float testJ = Lerp(cuspJM.x, limitJMax, testPositions[testIndex]);
		float slopeGain = GetFocusGain(testJ, analyticalThreshold, limitJMax, focusDistance);
		float intersectJ = SolveIntersectJ(testJ, cuspJM.y, focusJ, limitJMax, slopeGain);
		float slope = ComputeCompressionVectorSlope(intersectJ, focusJ, limitJMax, slopeGain);
		float cuspJ = SolveIntersectJ(cuspJM.x, cuspJM.y, focusJ, limitJMax, slopeGain);

		// Store values in parallel arrays
		testJMhs[testIndex] = { testJ, cuspJM.y, hue };
		intersectSourceJs[testIndex] = intersectJ;
		slopes[testIndex] = slope;
		intersectCuspJs[testIndex] = cuspJ;
	}

}

std::unique_ptr<aces::ReachMTable> aces::MakeReachMTable(const JMhParams& reachParams, float limitJMax) {
	const float searchRange = 50.0f;
	const float searchMaximum = 1300.0f;

	auto tablePtr = std::make_unique<ReachMTable>();
	ReachMTable& table = *tablePtr;
	for (uint32_t tableIndex = 0; tableIndex < tableSize; ++tableIndex) {
		// Lib.Academy.OutputTransform.ctl::make_reach_m_table, aces-core.
		float hue = BaseHueForPosition(tableIndex);
		float low = 0.0f;
		float high = low + searchRange;
		bool outside = false;

		while ((outside != true) && (high < searchMaximum)) {
			float3 searchJMh = {limitJMax, high, hue};
			float3 newLimitRGB = JMhToRGB(searchJMh, reachParams);
			//outside = any(newLimitRGB < 0.0);
			outside = newLimitRGB.x < 0.0f || newLimitRGB.y < 0.0f || newLimitRGB.z < 0.0f;
			if (!outside) {
				low = high;
				high = high + searchRange;
			}
		}

		while (high - low > 1e-2) {
			float sampleM = (high + low) / 2.0f;
			float3 searchJMh = {limitJMax, sampleM, hue};
			float3 newLimitRGB = JMhToRGB(searchJMh, reachParams);
			outside = newLimitRGB.x < 0.0f || newLimitRGB.y < 0.0f || newLimitRGB.z < 0.0f;
			if (outside) {
				high = sampleM;
			}
			else {
				low = sampleM;
			}
		}

		table[static_cast<size_t>(tableIndex) + 1] = high;
	}

	// Copy last populated entry to first empty spot
	table[0] = table[tableSize];

	// Copy first populated entry to last empty spot
	table[static_cast<size_t>(paddedTableSize) - 1] = table[1];

	return tablePtr;
}

std::unique_ptr<aces::GamutCuspsTable> aces::MakeUniformHueGamutTable(
	const JMhParams& reachParams, 
	const JMhParams& limitParams, 
	float peakLuminance, 
	float limitJMax, 
	float toneForwardLimit) {
	// The principal here is to sample the hues as uniformly as possible, whilst
	// ensuring we sample the corners of the limiting gamut and the reach
	// primaries at limit J Max
	//
	// The corners are calculated then the hues are extracted and merged to form
	// a unique sorted hue list We then build the hue table from the list, those
	// hues are then used to compute the JMh of the limiting gamut cusp.

	CuspCorners reachJMhCorners = FindReachCorners(reachParams, limitJMax, toneForwardLimit);
	CuspCorners limitingRGBCorners;
	CuspCorners limitingJMhCorners;

	BuildLimitingCuspCorners(limitingRGBCorners, limitingJMhCorners, limitParams, peakLuminance);
	SortedCornerHues sortedHues = ExtractSortedCubeHues(reachJMhCorners, limitingJMhCorners);

	auto hueTablePtr = std::make_unique<HueTable>();
	BuildHueTable(*hueTablePtr, sortedHues);

	auto cuspsTablePtr = std::make_unique<GamutCuspsTable>();
	BuildCuspsTable(*cuspsTablePtr, *hueTablePtr, limitingRGBCorners, limitingJMhCorners, limitParams);

	return cuspsTablePtr;
}

std::unique_ptr<aces::UpperHullGammaTable> aces::MakeUpperHullGammaTable(
	const GamutCuspsTable& gamutCuspsTable, 
	const JMhParams& limitParams, 
	float peakLuminance, 
	float limitJMax, 
	float midJ, 
	float focusDistance, 
	float invLowerHullGamma
) {
	auto tablePtr = std::make_unique<UpperHullGammaTable>();
	auto& table = *tablePtr;

	// Find upper hull gamma values for the gamut mapper.
	// Start by taking a h angle
	// Get the cusp J value for that angle
	// Find a J value halfway to the Jmax
	// Iterate through gamma values until the approximate max M is
	// negative through the actual boundary

	// positions between the cusp and Jmax we will check variables that get
	// set as we iterate through, once all are set to true we break the loop
	constexpr float gammaAccuracy = 1e-5f;

	for (int32_t i = 1; i < tableSize + 1; ++i) {
		// Get cusp from cusp table at hue position
		float4 cuspJMhx = gamutCuspsTable[i];
		float hue = cuspJMhx.z;
		float2 cuspJM = { cuspJMhx.x, cuspJMhx.y };

		std::array<float3, upperHullGammaTestCount> testJMhs{};
		std::array<float, upperHullGammaTestCount> intersectSourceJs{};
		std::array<float, upperHullGammaTestCount> slopes{};
		std::array<float, upperHullGammaTestCount> intersectCuspJs{};

		GenerateGammaTestData(cuspJM, hue, limitJMax, midJ, focusDistance,testJMhs, intersectSourceJs, slopes, intersectCuspJs);

		constexpr float searchRange = 0.4f;
		float low = 0.0f; // gamma_minimum
		float high = low + searchRange;
		while (high < 5.0f/*gamma_maximum*/) {
			// TODO: >=325 error, gamutCuspsTable fixed from 278...
			bool gammaFound = EvaluateGammaFit(
				cuspJM,
				1.0f / high,
				peakLuminance, 
				limitJMax, 
				invLowerHullGamma, 
				testJMhs,
				intersectSourceJs,
				slopes,
				intersectCuspJs,
				limitParams
			);
			if (!gammaFound) {
				low = high;
				high += searchRange;
			}
			else {
				break;
			}
		}

		float testGamma = -1.0;
		while ((high - low) > gammaAccuracy) {
			testGamma = MidPoint(high, low);
			bool gammaFound = EvaluateGammaFit(
				cuspJM,
				1.0f / testGamma,
				peakLuminance, 
				limitJMax, 
				invLowerHullGamma,
				testJMhs,
				intersectSourceJs,
				slopes,
				intersectCuspJs,
				limitParams
			);
			if (gammaFound) {
				high = testGamma;
			}
			else {
				low = testGamma;
			}
		}

		table[i] = 1.0f / high;
	}

	// Copy last populated entry to first empty spot
	table[0] = table[tableSize];

	// Copy first populated entry to last empty spot
	table[static_cast<size_t>(paddedTableSize) - 1] = table[1];

	return tablePtr;
}

aces::int2 aces::DetermineHueLinearitySearchRange(const GamutCuspsTable& gamutCuspsTable) {
	// This function searches through the hues looking for the largest
	// deviations from a linear distribution. We can then use this to initialise
	// the binary search range to something smaller than the full one to reduce
	// the number of lookups per hue lookup from ~ceil(log2(table size)) to
	// ~ceil(log2(range)) during image rendering.

	const int lowerPadding = 0;
	const int upperPadding = 1;

	int2 hueLinearitySearchRange = { lowerPadding, upperPadding };

	for (int i = 1; i < tableSize + 1; ++i) {
		const int pos = HuePositionInUniformTable(gamutCuspsTable[i].z, paddedTableSize);
		const int delta = i - pos;
		hueLinearitySearchRange = {
			std::min(hueLinearitySearchRange.x, delta + lowerPadding), 
			std::max(hueLinearitySearchRange.y, delta + upperPadding)
		};
	}
	return hueLinearitySearchRange;
}

aces::JMhParams::JMhParams(const Chromaticities& chromaticities) noexcept {
	// @note: GLM build scalars by column-first order, should not invoke the transpose() for CTL matrices.
	constexpr float3x3 baseConeResponseToAab = {
		2.0f, 1.0f, 1.0f / 9.0f,
		1.0f, -12.0f / 11.0f, 1.0f / 9.0f,
		1.0f / 20.0f, 1.0f / 11.0f, -2.0f / 9.0f
	};

	const float3x3 cam16ToXYZMat = chromaticitiesCAM16.makeRGBToXYZMat();
	const float3x3 xyzToCAM16Mat = glm::inverse(cam16ToXYZMat);
	const float3x3 rgbToXYZ = chromaticities.makeRGBToXYZMat(); // targetGamutRGB to XYZ

	const float3 whiteXYZ = rgbToXYZ * float3(cam_ref_luminance, cam_ref_luminance, cam_ref_luminance);

	// Step 0 - Converting CIE XYZ tristimulus values to sharpened RGB values
	float3 whiteCAM16 = xyzToCAM16Mat * whiteXYZ;

	const float3 D_RGB = {
		F_L_n * whiteXYZ.y / whiteCAM16.x,
		F_L_n * whiteXYZ.y / whiteCAM16.y,
		F_L_n * whiteXYZ.y / whiteCAM16.z
	};

	const float3 RGB_wc = {
		D_RGB.x * whiteCAM16.x,
		D_RGB.y * whiteCAM16.y,
		D_RGB.z * whiteCAM16.z
	};

	const float3 RGB_Aw = {
		PostAdaptationConeResponseCompressionForward(RGB_wc.x),
		PostAdaptationConeResponseCompressionForward(RGB_wc.y),
		PostAdaptationConeResponseCompressionForward(RGB_wc.z)
	};

	coneResponseToAab = baseConeResponseToAab * (identityMat3x3 * cam_nl_scale);
	float A_w = coneResponseToAab[0][0] * RGB_Aw.x + coneResponseToAab[1][0] * RGB_Aw.y + coneResponseToAab[2][0] * RGB_Aw.z;


	A_w_J = PostAdaptationConeResponseCompressionForwardImpl(F_L);
	inv_A_w_J = 1.0f / A_w_J;

	// Prescale the CAM16 LMS responses to directly provide for chromatic adaptation
	float3x3 M1 = xyzToCAM16Mat * rgbToXYZ;
	float3x3 M2 = identityMat3x3 * cam_ref_luminance;
	float3x3 rgbToCAM16 = M2 * M1;
	rgbToCAM16c = ScaleMatrixDiagonal(identityMat3x3, D_RGB) * rgbToCAM16;
	cam16cToRGB = glm::inverse(rgbToCAM16c);

	coneResponseToAab = {
		coneResponseToAab[0][0] / A_w, coneResponseToAab[0][1] * 43.0f * cam_surround.z, coneResponseToAab[0][2] * 43.0f * cam_surround.z,
		coneResponseToAab[1][0] / A_w, coneResponseToAab[1][1] * 43.0f * cam_surround.z, coneResponseToAab[1][2] * 43.0f * cam_surround.z,
		coneResponseToAab[2][0] / A_w, coneResponseToAab[2][1] * 43.0f * cam_surround.z, coneResponseToAab[2][2] * 43.0f * cam_surround.z
	};

	aabToConeResponse = glm::inverse(coneResponseToAab);
}

aces::TonescaleParams::TonescaleParams(float inPeakLuminance) noexcept {
	// Preset constants that set the desired behavior for the curve
	peakLuminance = inPeakLuminance;

	n_r = 100.0f;					    // normalized white in nits (what 1.0 should be)
	g = 1.15f;						    // surround / contrast
	constexpr float c = 0.18f;		    // anchor for 18% grey
	constexpr float c_d = 10.013f;	    // output luminance of 18% grey (in nits)
	constexpr float w_g = 0.14f;	    // change in grey between different peak luminance
	t_1 = 0.04f;					    // shadow toe or flare/glare compensation
	constexpr float r_hit_min = 128.0f; // scene-referred value "hitting the roof" for SDR (e.g. when n = 100 nits)
	constexpr float r_hit_max = 896.0f; // scene-referred value "hitting the roof" for when n = 10000 nits

	// Calculate output constants
	const float r_hit = r_hit_min + (r_hit_max - r_hit_min) * (log(peakLuminance / n_r) / log(10000.0f / 100.0f));
	const float m_0 = (peakLuminance / n_r);
	const float m_1 = 0.5f * (m_0 + sqrt(m_0 * (m_0 + 4.0f * t_1)));
	const float u = pow((r_hit / m_1) / ((r_hit / m_1) + 1.0f), g);
	const float m = m_1 / u;
	const float w_i = log(peakLuminance / 100.0f) / log(2.0f);
	c_t = c_d / n_r * (1.0f + w_i * w_g);

	const float g_ip = 0.5f * (c_t + sqrt(c_t * (c_t + 4.0f * t_1)));
	const float g_ipp2 = -(m_1 * pow((g_ip / m), (1.0f / g))) / (pow(g_ip / m, 1.0f / g) - 1.0f);
	const float w_2 = c / g_ipp2;
	s_2 = w_2 * m_1;
	u_2 = pow((r_hit / m_1) / ((r_hit / m_1) + w_2), g);
	m_2 = m_1 / u_2;

	forwardLimit = 8.0f * r_hit;
	inverseLimit = peakLuminance / (u_2 * n_r);
	logPeak = log10(m_0);
}

aces::OutputTransformParams::OutputTransformParams(float inPeakLuminance, const Chromaticities& limitChromaticities) noexcept : 
	// JMh parameters
	// We are making rendering application, let the input from AP1 directly.
	inputParams(chromaticitiesACEScg/*chromaticitiesACES2065_1*/),
	reachParams(chromaticitiesACEScg), 
	limitParams(limitChromaticities), 
	// Tonescale parameters
	tonescaleParams(inPeakLuminance)
{
	peakLuminance = inPeakLuminance;

	// Shared compression paramters
	limitJMax = YToJ(peakLuminance, inputParams);
	invModelGamma = 1.0f / cam_model_gamma;

	// Chroma compression parameters
	saturation = std::max(0.2f, chroma_expand - (chroma_expand * chroma_expand_fact) * tonescaleParams.logPeak);
	saturationThreshold = chroma_expand_thr / peakLuminance;
	chromaCompression = chroma_compress + (chroma_compress * chroma_compress_fact) * tonescaleParams.logPeak;
	chromaCompressionScale = pow(0.03379f * peakLuminance, 0.30596f) - 0.45135f;

	// Gamut compression parameters
	midJ = YToJ(tonescaleParams.c_t * cam_ref_luminance, inputParams);
	focusDistiance = focus_distance + focus_distance * focus_distance_scaling * tonescaleParams.logPeak;
	const float lowerHullGamma = 1.14f + 0.07f * tonescaleParams.logPeak;
	invLowerHullGamma = 1.0f / lowerHullGamma;

	// Precompiled-tables
	reachMTable = MakeReachMTable(reachParams, limitJMax);
	gamutCuspsTable = MakeUniformHueGamutTable(
		reachParams, limitParams, peakLuminance, limitJMax, tonescaleParams.forwardLimit
	);
	// Redundancy.
	//for (int i = 0; i != totalTableSize; i = i + 1) {
	//	p.TABLE_hues[i] = p.TABLE_gamut_cusps[i][2];
	//}
	upperHullGammaTable = MakeUpperHullGammaTable(
		*gamutCuspsTable, limitParams, peakLuminance, limitJMax, midJ, focusDistiance, invLowerHullGamma
	);

	hueLinearitySearchRange = DetermineHueLinearitySearchRange(*gamutCuspsTable);
}

const aces::OutputTransformParamsSSBO aces::OutputTransformParamsWrapper::takeSSBO() {
	verifyTakenFlag(_ssboTaken);
	OutputTransformParamsSSBO ssbo {
		_params->peakLuminance, 
		_params->limitJMax, 
		_params->invModelGamma, 
		_params->saturation, 
		_params->saturationThreshold, 
		_params->chromaCompression, 
		_params->chromaCompressionScale, 
		_params->midJ, 
		_params->focusDistiance, 
		_params->invLowerHullGamma, 
		_params->hueLinearitySearchRange,

		_params->tonescaleParams.n_r, 
		_params->tonescaleParams.g,
		_params->tonescaleParams.t_1,
		_params->tonescaleParams.c_t,
		_params->tonescaleParams.s_2,
		_params->tonescaleParams.u_2,
		_params->tonescaleParams.m_2, 
		_params->tonescaleParams.forwardLimit,
		_params->tonescaleParams.inverseLimit,

		JMhParams::F_L_n, 
		JMhParams::cz, 
		JMhParams::inv_cz, 

		_params->inputParams.A_w_J,
		_params->inputParams.inv_A_w_J,
		_params->inputParams.rgbToCAM16c, 
		_params->inputParams.coneResponseToAab, 
		_params->limitParams.cam16cToRGB, 
		_params->limitParams.aabToConeResponse
	};
	updateTakenFlag(_ssboTaken);
	return ssbo;
}

const std::unique_ptr<aces::ReachMTable> aces::OutputTransformParamsWrapper::takeReachMTable() {
	verifyTakenFlag(_reachMTableTaken);
	std::unique_ptr<aces::ReachMTable> ptr = std::move(_params->reachMTable);
	updateTakenFlag(_reachMTableTaken);
	return std::move(ptr);
}

const std::unique_ptr<aces::UpperHullGammaTable> aces::OutputTransformParamsWrapper::takeUpperHullGammaTable() {
	verifyTakenFlag(_upperHullGammaTableTaken);
	std::unique_ptr<UpperHullGammaTable> ptr = std::move(_params->upperHullGammaTable);
	updateTakenFlag(_upperHullGammaTableTaken);
	return std::move(ptr);
}

const std::unique_ptr<aces::GamutCuspsTable> aces::OutputTransformParamsWrapper::takeGamutCuspsTable() {
	verifyTakenFlag(_gamutCuspsTableTaken);
	std::unique_ptr<GamutCuspsTable> ptr = std::move(_params->gamutCuspsTable);
	updateTakenFlag(_gamutCuspsTableTaken);
	return std::move(ptr);
}

void aces::OutputTransformParamsWrapper::verifyTakenFlag(bool& flag) {
	// Recreate resource if it is not exists.
	if (!_params) {
		_params = std::make_unique<OutputTransformParams>();
		_ssboTaken = false;
		_reachMTableTaken = false;
		_upperHullGammaTableTaken = false;
		_gamutCuspsTableTaken = false;
	}
	if (flag) {
		throw std::runtime_error("[OutputTransformParamsWrapper] Resource had been taken.");
	}
}

void aces::OutputTransformParamsWrapper::updateTakenFlag(bool& flag) {
	flag = true;
	// Clear resource for memory saving.
	if (_ssboTaken && _reachMTableTaken && _upperHullGammaTableTaken && _gamutCuspsTableTaken) {
		_params.reset();
	}
}
