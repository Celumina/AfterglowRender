#pragma once
#include <glm/glm.hpp>

namespace aces {
	using float2 = glm::vec2;
	using float3 = glm::vec3;
	using float4 = glm::vec4;

	using float2x2 = glm::mat2x2;
	using float3x3 = glm::mat3x3;
	using float3x4 = glm::mat3x4;
	using float4x4 = glm::mat4x4;

	using int2 = glm::ivec2;
	using int3 = glm::ivec3;
	using int4 = glm::ivec4;

	/**
	* @brief: Chromaticity Coordinates Struct
	* ..primary.x = CIE_X / (CIE_X + CIE_Y + CIE_Z)
	* ..primary.y = CIE_Y / (CIE_X + CIE_Y + CIE_Z)
	*/
	using Primary = float2;

	constexpr Primary whiteCIE1931_D65 = { 0.3127f, 0.3290f };
	constexpr Primary whiteACES_D60 = { 0.32168f, 0.33767f };
	constexpr Primary whiteDCI_Calibration = { 0.314f, 0.351f };
	constexpr Primary whiteCAM16 = { 0.333f, 0.333f };

	struct Chromaticities {
		Primary red;
		Primary green;
		Primary blue;
		Primary white;

		float3x3 makeRGBToXYZMat() const noexcept;
		// float3x3 makeXYZToRGBMat() noexcept;
	};

	// AP0
	constexpr Chromaticities chromaticitiesACES2065_1 {
		{ 0.73470f, 0.26530f }, 
		{ 0.00000f, 1.00000f },
		{ 0.00010f, -0.07700f },
		whiteACES_D60
	};

	// AP1
	constexpr Chromaticities chromaticitiesACEScg{
		{ 0.713f, 0.293f },
		{ 0.165f, 0.830f },
		{ 0.128f, 0.044f },
		whiteACES_D60
	};

	constexpr Chromaticities chromaticitiesCAM16 {
		{ 0.8336f, 0.1735f },
		{ 2.3854f, -1.4659f },
		{ 0.087f, -0.125f },
		whiteCAM16
	};

	// Output device Gamuts
	constexpr Chromaticities chromaticitiesSRGB {
		{ 0.64f, 0.33f },
		{ 0.30f, 0.60f },
		{ 0.15f, 0.06f },
		whiteCIE1931_D65
	};

	constexpr Chromaticities chromaticitiesRec2020{
		{ 0.708f, 0.292f },
		{ 0.170f, 0.797f },
		{ 0.131f, 0.046f },
		whiteCIE1931_D65
	};

	constexpr Chromaticities chromaticitiesP3_DCI{
		{ 0.680f, 0.320f },
		{ 0.265f, 0.690f },
		{ 0.150f, 0.060f },
		whiteDCI_Calibration
	};

	constexpr Chromaticities chromaticitiesP3_D65{
		{ 0.680f, 0.320f },
		{ 0.265f, 0.690f },
		{ 0.150f, 0.060f },
		whiteCIE1931_D65
	};

	// TODO: Use Chromaticities instead?
	/**
	* @note: 
	*	HLSL: Column-Major, Constructor fill scalars by rows   (manuscript intuition), Fetch[row][column]
	*	GLM:  Column-Major, Constructor fill scalars by column (memory layout based),  Fetch[column][row]
	*	CTL:  Row-Major,    Constructor fill scalars by rows   (both fit),             Fetch[row][column]
	* 
	* @example: 
	*	in HLSL:
	*		float3 a = mul(XYZToAP1Mat, float3(0.3, 0.5, 0.7));
	*		float3 b = float3(XYZToAP1Mat[0][0], XYZToAP1Mat[0][1], XYZToAP1Mat[0][2]);
	*	in GLM to get the same result:
	*		auto m = glm::transpose(XYZToAP1Mat);
	*		auto a = m * float3(0.3, 0.5, 0.7);
	*		auto b = float3(m[0][0], m[1][0], m[2][0]);
	* 
	*	for a matrix construction m = {1, 2, 3, 4, 5, 6, 7, 8, 9} in CTL, 
	*	the GLM matrix constructor can using this scalar order directly for column-marjor calculation.
	*	and the same subsciption e.g. [1][2] also yields same scalar result.
	* 
	*/ 

	template<
		float m00, float m01, float m02,
		float m10, float m11, float m12,
		float m20, float m21, float m22
	>
	constexpr float3x3 HLSLFloat3x3() noexcept {
		return float3x3{
			m00, m10, m20,
			m01, m11, m21,
			m02, m12, m22,
		};
	}

	//constexpr float3x3 AP0ToXYZMat = HLSLFloat3x3<
	//	0.9525523959f, 0.0000000000f, 0.0000936786f,
	//	0.3439664498f, 0.7281660966f,-0.0721325464f,
	//	0.0000000000f, 0.0000000000f, 1.0088251844f
	//>();

	//constexpr float3x3 XYZToAP0Mat = HLSLFloat3x3<
	//	 1.0498110175f, 0.0000000000f,-0.0000974845f,
	//	-0.4959030231f, 1.3733130458f, 0.0982400361f,
	//	 0.0000000000f, 0.0000000000f, 0.9912520182f
	//>();

	//constexpr float3x3 AP1ToXYZMat = HLSLFloat3x3<
	//	 0.6624541811f, 0.1340042065f, 0.1561876870f,
	//	 0.2722287168f, 0.6740817658f, 0.0536895174f,
	//	-0.0055746495f, 0.0040607335f, 1.0103391003f
	//>();

	//constexpr float3x3 XYZToAP1Mat = HLSLFloat3x3<
	//	 1.6410233797f, -0.3248032942f, -0.2364246952f,
	//	-0.6636628587f,  1.6153315917f,  0.0167563477f,
	//	 0.0117218943f, -0.0082844420f,  0.9883948585f
	//>();

	////mul( AP0ToXYZMat, XYZToAP1Mat );
	//constexpr float3x3 AP0ToAP1Mat = HLSLFloat3x3<
	//	 1.4514393161f, -0.2365107469f, -0.2149285693f,
	//	-0.0765537734f,  1.1762296998f, -0.0996759264f,
	//	 0.0083161484f, -0.0060324498f,  0.9977163014f
	//>();

	////mul( AP1ToXYZMat, XYZToAP0Mat );
	//constexpr float3x3 AP1ToAP0Mat = HLSLFloat3x3<
	//	 0.6954522414f,  0.1406786965f,  0.1638690622f,
	//	 0.0447945634f,  0.8596711185f,  0.0955343182f,
	//	-0.0055258826f,  0.0040252103f,  1.0015006723f
	//>();

	//// REC 709 primaries
	//constexpr float3x3 XYZToSRGBMat = HLSLFloat3x3<
	//	 3.2409699419f, -1.5373831776f, -0.4986107603f,
	//	-0.9692436363f,  1.8759675015f,  0.0415550574f,
	//	 0.0556300797f, -0.2039769589f,  1.0569715142f
	//>();

	//constexpr float3x3 sRGBToXYZMat = HLSLFloat3x3<
	//	0.4123907993f, 0.3575843394f, 0.1804807884f,
	//	0.2126390059f, 0.7151686788f, 0.0721923154f,
	//	0.0193308187f, 0.1191947798f, 0.9505321522f
	//>();

	//// REC 2020 primaries
	//constexpr float3x3 XYZToRec2020Mat = HLSLFloat3x3<
	//	 1.7166511880f, -0.3556707838f, -0.2533662814f,
	//	-0.6666843518f,  1.6164812366f,  0.0157685458f,
	//	 0.0176398574f, -0.0427706133f,  0.9421031212f
	//>();

	//constexpr float3x3 Rec2020ToXYZMat = HLSLFloat3x3<
	//	0.6369580483f, 0.1446169036f, 0.1688809752f,
	//	0.2627002120f, 0.6779980715f, 0.0593017165f,
	//	0.0000000000f, 0.0280726930f, 1.0609850577f
	//>();

	//// P3, D65 primaries
	//constexpr float3x3 XYZToP3D65Mat = HLSLFloat3x3<
	//	 2.4934969119f, -0.9313836179f, -0.4027107845f,
	//	-0.8294889696f,  1.7626640603f,  0.0236246858f,
	//	 0.0358458302f, -0.0761723893f,  0.9568845240f
	//>();

	//constexpr float3x3 P3D65ToXYZMat = HLSLFloat3x3<
	//	0.4865709486f, 0.2656676932f, 0.1982172852f,
	//	0.2289745641f, 0.6917385218f, 0.0792869141f,
	//	0.0000000000f, 0.0451133819f, 1.0439443689f
	//>();

	//// CAT: Chromatic Adaptation Transform
	//// Bradford chromatic adaptation transforms between ACES white point (D60) and sRGB white point (D65)
	//constexpr float3x3 D65ToD60CAT = HLSLFloat3x3<
	//	 1.0130349146f, 0.0061052578f, -0.0149709436f,
	//	 0.0076982301f, 0.9981633521f, -0.0050320385f,
	//	-0.0028413174f, 0.0046851567f,  0.9245061375f
	//>();

	//constexpr float3x3 D60ToD65CAT = HLSLFloat3x3<
	//	 0.9872240087f, -0.0061132286f, 0.0159532883f,
	//	-0.0075983718f,  1.0018614847f, 0.0053300358f,
	//	 0.0030725771f, -0.0050959615f, 1.0816806031f
	//>();

	//// CAM16 (Color Appearance Model 2016) 
	//constexpr float3x3 CAM16ToXYZMat = HLSLFloat3x3<
	//	 2.0512756811f, -1.1400313439f,  0.0887556628f,
	//	 0.4269389763f,  0.7005835277f, -0.1275225040f,
	//	-0.0174712779f, -0.0384725929f,  1.0589468739f
	//>();

	//constexpr float3x3 XYZToCAM16Mat = HLSLFloat3x3<
	//	 0.3640744835f,  0.5947008156f, 0.04110127349f,
	//	-0.2222450987f,  1.0738554823f, 0.14794533610f,
	//	-0.0020676190f,  0.0488260453f, 0.95038755696f
	//>();

	constexpr float3x3 identityMat3x3 = HLSLFloat3x3<
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	>();
}