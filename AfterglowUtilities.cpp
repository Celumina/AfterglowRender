#include "AfterglowUtilities.h"

#include <algorithm>
#include <string>
#include <locale>

std::wstring util::ToWstring(const std::string& str) {
    // const char* source = str.c_str();
    size_t requiredSize = 0;
    mbstowcs_s(&requiredSize, nullptr, 0, str.data(), _TRUNCATE);
    std::wstring wstr(requiredSize, L'\0');
    mbstowcs_s(nullptr, wstr.data(), requiredSize, str.data(), _TRUNCATE);
    return wstr;
}

std::string util::ToString(const std::wstring& wstr) {
    // Done in main() of this program.
    // std::setlocale(LC_ALL, "en_US.utf8"); 

    size_t requiredSize = 0;
    wcstombs_s(&requiredSize, nullptr, 0, wstr.data(), _TRUNCATE);
    std::string str(requiredSize, '\0');
    wcstombs_s(nullptr, str.data(), requiredSize, wstr.data(), _TRUNCATE);
    return str;
}

std::string util::UpperCase(const std::string& str) {
    std::string upperStr = str;
    std::transform(str.begin(), str.end(), upperStr.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return upperStr;
}

size_t util::Align(size_t value, size_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}
