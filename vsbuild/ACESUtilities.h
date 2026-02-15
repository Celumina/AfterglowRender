#pragma once

#include <array>
#include <memory>
#include <gcem.hpp>
#include "ACESOutputDevice.h"
#include "TypeConstraints.h"

namespace aces {
	// Common constants
	constexpr float pi = 3.14159265359f;

	// Table constants
	constexpr float hueLimit = 360.0f;
	constexpr uint32_t tableSize = 360;
	constexpr uint32_t paddedTableSize = tableSize + 2;
	constexpr uint32_t cuspCornerCount = 6;
	constexpr uint32_t paddedCuspCornerCount = cuspCornerCount + 2;
	constexpr uint32_t maxSortedCornerCount = cuspCornerCount * 2;

	constexpr uint32_t upperHullGammaTestCount = 5;

	using HueTable = std::array<float, paddedTableSize>;
	using ReachMTable = std::array<float, paddedTableSize>;
	using UpperHullGammaTable = std::array<float, paddedTableSize>;
	using GamutCuspsTable = std::array<float4, paddedTableSize>; // float4 for memory alignment.

	using CuspCorners = std::array<float3, paddedCuspCornerCount>;
	using SortedCornerHues = std::array<float, maxSortedCornerCount>;

	// CAM Params
	constexpr float cam_ref_luminance = 100.0f;
	constexpr float cam_L_A = 100.0f;
	constexpr float cam_Y_b = 20.0f;
	constexpr float3 cam_surround = { 0.9f, 0.59f, 0.9f };

	constexpr float cam_J_scale = 100.0f;
	constexpr float cam_nl_Y_reference = 100.0f;
	constexpr float cam_nl_offset = 0.2713f * cam_nl_Y_reference;
	constexpr float cam_nl_scale = 4.0f * cam_nl_Y_reference;

	constexpr float cam_model_gamma = cam_surround.y * (1.48f + gcem::sqrt(cam_Y_b / cam_ref_luminance));

	// Chroma compression
	constexpr float chroma_compress = 2.4f;
	constexpr float chroma_compress_fact = 3.3f;
	constexpr float chroma_expand = 1.3f;
	constexpr float chroma_expand_fact = 0.69f;
	constexpr float chroma_expand_thr = 0.5f;

	// Gamut compression
	constexpr float smooth_cusps = 0.12f;
	constexpr float smooth_m = 0.27f;
	constexpr float cusp_mid_blend = 1.3f;

	constexpr float focus_gain_blend = 0.3f;
	constexpr float focus_adjust_gain = 0.55f;
	constexpr float focus_distance = 1.35f;
	constexpr float focus_distance_scaling = 1.75f;

	constexpr float compression_threshold = 0.75f;

	struct JMhParams {
		explicit JMhParams(const Chromaticities& chromaticities = OutputDeviceChromaticities()) noexcept;
		// Pre-computed conversion matrices and constants for conversions to/from JMh
		float3x3 rgbToCAM16c;
		float3x3 cam16cToRGB;
		float3x3 coneResponseToAab;
		float3x3 aabToConeResponse;

		// Viewing condition dependent parameters
		static constexpr float k = 1.0f / (5.0f * cam_L_A + 1.0f);
		static constexpr float k4 = k * k * k * k;
		static constexpr float F_L = 0.2f * k4 * (5.0f * cam_L_A) + 0.1f * gcem::pow((1.0f - k4), 2.0f) * gcem::pow(5.0f * cam_L_A, 1.0f / 3.0f);

		static constexpr float F_L_n = F_L / cam_ref_luminance; // F_L normalised
		static constexpr float cz = cam_model_gamma;
		static constexpr float inv_cz = 1.0f / cz; 

		float A_w_J;
		float inv_A_w_J; // 1/A_w_J
	};

	struct TonescaleParams {
		explicit TonescaleParams(float inPeakLuminance = outputDevicePeakLuminance) noexcept;
		float peakLuminance;
		float n_r;
		float g;
		float t_1;
		float c_t;
		float s_2;
		float u_2;
		float m_2;
		float forwardLimit;
		float inverseLimit;
		float logPeak;
	};

	// TODO: Transfer params to ssbo build by inreflect and structLayout?
	// TODO: Enum InitFormInternalFunction do not need to provide variables from material asset.
	struct OutputTransformParams {
		explicit OutputTransformParams(
			float inPeakLuminance = outputDevicePeakLuminance, 
			const Chromaticities& limitChromaticities = OutputDeviceChromaticities()
		) noexcept;

		float peakLuminance;

		// Shared compression parameters
		float limitJMax;
		float invModelGamma;

		// Chroma compression parameters
		float saturation;
		float saturationThreshold;
		float chromaCompression;
		float chromaCompressionScale;

		// Gamut compression parameters
		float midJ;
		float focusDistiance;
		float invLowerHullGamma;

		int2 hueLinearitySearchRange;

		// JMh parameters
		JMhParams inputParams;
		JMhParams reachParams;
		JMhParams limitParams;

		// Tonescale parameters
		TonescaleParams tonescaleParams;

		// @note: redundancy part: float TABLE_hues[totalTableSize];
		std::unique_ptr<ReachMTable> reachMTable;
		std::unique_ptr<GamutCuspsTable> gamutCuspsTable;
		std::unique_ptr<UpperHullGammaTable> upperHullGammaTable;
	};

	struct alignas(16) OutputTransformParamsSSBO {
		float peakLuminance;

		// Shared compression parameters
		float limitJMax;
		float invModelGamma;

		// Chroma compression parameters
		float saturation;
		float saturationThreshold;
		float chromaCompression;
		float chromaCompressionScale;

		// Gamut compression parameters
		float midJ;
		float focusDistiance;
		float invLowerHullGamma;

		int2 hueLinearitySearchRange;

		// Tonescale params
		float tonescale_n_r;
		float tonescale_g;
		float tonescale_t_1;
		float tonescale_c_t;
		float tonescale_s_2;
		float tonescale_u_2;
		float tonescale_m_2;
		float tonescale_forwardLimit;
		float tonescale_inverseLimit;

		// JMh params
		float jmh_F_L_n;
		float jmh_cz;
		float jmh_inv_cz;

		float inputJMh_A_w_j;
		float inputJMh_inv_A_w_j;

		// Use glm::mat3x4 for stroage buffer struct std140 alignment rules.
		alignas(16) float3x4 inputJMh_rgbToCam16c;
		alignas(16) float3x4 inputJMh_coneResponseToAab;
		alignas(16) float3x4 limitJMh_cam16cToRGB;
		alignas(16) float3x4 limitJMh_aabToConeResponse;
	};

	class OutputTransformParamWrapper {
	public:
		const OutputTransformParamsSSBO takeSSBO();
		const std::unique_ptr<ReachMTable> takeReachMTable();
		const std::unique_ptr<UpperHullGammaTable> takeUpperHullGammaTable();
		const std::unique_ptr<GamutCuspsTable> takeGamutCuspsTable();

	private:
		void verifyTakenFlag(bool& flag);
		void updateTakenFlag(bool& flag);

		std::unique_ptr<OutputTransformParams> _params;
		bool _ssboTaken = false;
		bool _reachMTableTaken = false;
		bool _upperHullGammaTableTaken = false;
		bool _gamutCuspsTableTaken = false;
	};

	OutputTransformParamWrapper& OutputTransformContext();

	// Base Functions 
	template<typename Type>
	constexpr Type Radians(Type degrees) noexcept { return degrees * static_cast<Type>(1.0 / 180.0 * pi); }

	template<typename Type>
	constexpr Type Degrees(Type radians) noexcept { return radians * static_cast<Type>(180.0 / pi); }

	template<typename Type>
	constexpr Type MidPoint(Type a, Type b) noexcept { return (a + b) * static_cast<Type>(0.5); }

	template<typename Type, typename WeightType>
	constexpr Type Lerp(Type a, Type b, WeightType t) { return a + (b - a) * t; }

	float WrapTo360(float x);
	float3x3 ScaleMatrixDiagonal(const float3x3& mat, const float3& scale) noexcept;
	float BaseHueForPosition(uint32_t index) noexcept;
	uint32_t HuePositionInUniformTable(float hue, uint32_t size) noexcept;

	// CAM functions 
	float PostAdaptationConeResponseCompressionForwardImpl(float Rc) noexcept;
	float PostAdaptationConeResponseCompressionForward(float v) noexcept;
	float PostAdaptationConeResponseCompressionInverseImpl(float Ra) noexcept;
	float PostAdaptationConeResponseCompressionInverse(float v) noexcept;

	// Color SpaceTransforms
	float JToAchromaticN(float J, float inv_cz) noexcept;
	float AchromaticNToJ(float A, float cz) noexcept;
	float YToJ(float Y, const JMhParams& params) noexcept;
	float3 AabToJMh(const float3& colorAab, const JMhParams& jmhParams) noexcept;
	float3 JMhToAab(const float3& colorJMh, const JMhParams& jmhParams) noexcept;
	float3 RGBToAab(const float3& colorRGB, const JMhParams& jmhParams) noexcept;
	float3 AabToRGB(const float3& colorAab, const JMhParams& jmhParams) noexcept;
	float3 RGBToJMh(const float3& colorRGB, const JMhParams& jmhParams) noexcept;
	float3 JMhToRGB(const float3& colorJMh, const JMhParams& jmhParams) noexcept;

	// Table generation functions
	float3 GenerateUnitCubeCuspCorners(uint32_t cornerIndex);
	void BuildLimitingCuspCorners(
		CuspCorners& limitingRGBCorners, CuspCorners& limitingJMhCorners, const JMhParams& limitParams, float peakLuminance
	);
	CuspCorners FindReachCorners(const JMhParams& reachParams, float limitJMax, float toneForwardLimit);
	SortedCornerHues ExtractSortedCubeHues(const CuspCorners& reachJMhCorners, const CuspCorners& limitJMhCorners);
	float2 FindDisplayCuspForHue(float hue, const CuspCorners& rgbCorners, const CuspCorners& jmhCorners, const JMhParams& jmhParams);
	void BuildHueSampleInterval(HueTable& hueTable, int32_t samples, float lower, float upper, int32_t baseIndex);
	void BuildHueTable(HueTable& hueTable, const SortedCornerHues& sortedHues);
	void BuildCuspsTable(
		GamutCuspsTable& cuspsTable, 
		const HueTable& hueTable, 
		const CuspCorners& rgbCorners, 
		const CuspCorners& jmhCornoers, 
		const JMhParams& jmhParams
	);
	float ComputeFocusJ(float cuspJ, float midJ, float limitJMax) noexcept;
	float GetFocusGain(float colorJ, float analyticalThreshold, float limitJMax, float focusDistance) noexcept;
	float SolveIntersectJ(float colorJ, float colorM, float focusJ, float maxJ, float slopeGain) noexcept;
	float ComputeCompressionVectorSlope(float intersectJ, float focusJ, float limitJMax, float slopeGain) noexcept;
	float SmoothMinimumScaled(float a, float b, float scaleReference) noexcept;
	float EstimateLineAndBoundaryIntersectionM(
		float axisIntersectJ, 
		float slope, 
		float invGamma, 
		float maxJ, 
		float maxM, 
		float intersectionReferenceJ
	) noexcept;
	float FindGamutBoundaryIntersection(
		float2 cuspJM, 
		float maxJ, 
		float invGammaTop, 
		float invGammaBottom, 
		float intersectSourceJ, 
		float slope, 
		float intersectCuspJ
	) noexcept;
	bool EvaluateGammaFit(
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
	);
	void GenerateGammaTestData(
		const float2 cuspJM, 
		const float hue, 
		const float limitJMax, 
		const float midJ, 
		const float focusDistance, 
		std::array<float3, upperHullGammaTestCount>& testJMhs,
		std::array<float, upperHullGammaTestCount>& intersectSourceJs, 
		std::array<float, upperHullGammaTestCount>& slopes,
		std::array<float, upperHullGammaTestCount>& intersectCuspJs
	);
	std::unique_ptr<ReachMTable> MakeReachMTable(const JMhParams& reachParams, float limitJMax);
	std::unique_ptr<GamutCuspsTable> MakeUniformHueGamutTable(
		const JMhParams& reachParams, 
		const JMhParams& limitParams, 
		float peakLuminance, 
		float limitJMax, 
		float toneForwardLimit
	);
	std::unique_ptr<UpperHullGammaTable> MakeUpperHullGammaTable(
		const GamutCuspsTable& gamutCuspsTable, 
		const JMhParams& limitParams, 
		float peakLuminance, 
		float limitJMax, 
		float midJ, 
		float focusDistance, 
		float invLowerHullGamma
	);
	int2 DetermineHueLinearitySearchRange(const GamutCuspsTable& gamutCuspsTable);
}
