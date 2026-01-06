#pragma once

#include "ACESCommon.h"

// Display Settings
// Luminance the tone scale highlight rolloff will traget in cd/m^2 (nits)
namespace aces {
	enum class EOTF {
		Linear, 
		ST_2084, 
		HLG, 
		Gamma_2_6, 
		BT_1886_With_Gamma_2_4, 
		Gamma_2_2, 
		sRGB_IEC_61966_2_1_1999, 

		EnumCount
	};

	enum class Gamut {
		sRGB_D65, 
		DCI_P3_D65, 
		Rec2020_D65, 
		ACES_D60, 
		ACEScg_D60
	};

	// TODO: Get these configs from system.
	constexpr float outputDevicePeakLuminance = 100.0f;
	constexpr EOTF outputDeviceEOTF = EOTF::sRGB_IEC_61966_2_1_1999;
	constexpr Gamut outputDeviceGamut = Gamut::sRGB_D65;

	constexpr Chromaticities OutputDeviceChromaticities() noexcept;
}

constexpr aces::Chromaticities aces::OutputDeviceChromaticities() noexcept {
	if constexpr (outputDeviceGamut == Gamut::sRGB_D65) {
		return chromaticitiesSRGB;
	}
	else if constexpr (outputDeviceGamut == Gamut::DCI_P3_D65) {
		return chromaticitiesP3_D65;
	}
	else if constexpr (outputDeviceGamut == Gamut::Rec2020_D65) {
		return chromaticitiesRec2020;
	}
	else {
		// TODO: Find a way to implement a valid assert in compile time.
		// static_assert(false, "Unsupported output device gamut.");
		return chromaticitiesSRGB;
	}
}
