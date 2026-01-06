#pragma once
#include <format>

namespace asniColor {
	using Text = const char*;

	// fore (Text) Colors
	static constexpr Text foreBlack = "\033[30m";
	static constexpr Text foreRed = "\033[31m";
	static constexpr Text foreGreen = "\033[32m";
	static constexpr Text foreYellow = "\033[33m";
	static constexpr Text foreBlue = "\033[34m";
	static constexpr Text foreMagenta = "\033[35m";
	static constexpr Text foreCyan = "\033[36m";
	static constexpr Text foreWhite = "\033[37m";

	// Bright fore Colors
	static constexpr Text foreBrightBlack = "\033[90m";
	static constexpr Text foreBrightRed = "\033[91m";
	static constexpr Text foreBrightGreen = "\033[92m";
	static constexpr Text foreBrightYellow = "\033[93m";
	static constexpr Text foreBrightBlue = "\033[94m";
	static constexpr Text foreBrightMagenta = "\033[95m";
	static constexpr Text foreBrightCyan = "\033[96m";
	static constexpr Text foreBrightWhite = "\033[97m";

	// back Colors
	static constexpr Text backBlack = "\033[40m";
	static constexpr Text backRed = "\033[41m";
	static constexpr Text backGreen = "\033[42m";
	static constexpr Text backYellow = "\033[43m";
	static constexpr Text backBlue = "\033[44m";
	static constexpr Text backMagenta = "\033[45m";
	static constexpr Text backCyan = "\033[46m";
	static constexpr Text backWhite = "\033[47m";

	// Bright back Colors
	static constexpr Text backBrightBlack = "\033[100m";
	static constexpr Text backBrightRed = "\033[101m";
	static constexpr Text backBrightGreen = "\033[102m";
	static constexpr Text backBrightYellow = "\033[103m";
	static constexpr Text backBrightBlue = "\033[104m";
	static constexpr Text backBrightMagenta = "\033[105m";
	static constexpr Text backBrightCyan = "\033[106m";
	static constexpr Text backBrightWhite = "\033[107m";

	// Formatting Options
	static constexpr Text reset = "\033[0m";
	static constexpr Text bold = "\033[1m";
	static constexpr Text dim = "\033[2m";
	static constexpr Text italic = "\033[3m";
	static constexpr Text underline = "\033[4m";
	static constexpr Text blink = "\033[5m";
	static constexpr Text reverse = "\033[7m";
	static constexpr Text hidden = "\033[8m";
	static constexpr Text strikethrough = "\033[9m";

	// ====================
	// Helper Functions
	// ====================

	/**
	* @brief Wraps text in ANSI color codes.
	* @param text The text to color.
	* @param color The ANSI color code (e.g., ansi::foreRed).
	* @return The colored text (std::string).
	*/
	inline std::string Colorize(const std::string& text, const char* color) {
		return std::format("{}{}{}", color, text, reset);
	}
  }