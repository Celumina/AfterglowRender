#ifndef ACES_TABLE_COMMON_HLSL
#define ACES_TABLE_COMMON_HLSL

#include "../Common.hlsl"

static const uint tableSize = 360;
static const uint paddedTableSize = tableSize + 2;
static const float hueLimit = 360.0;

uint HuePositionInUniformTable(float hue, uint size = tableSize) {
	const float wrappedHue = WrapTo360(hue);
	// Truncated into [0, 359]
	return wrappedHue / hueLimit * size;
}

// int NextPositionInTable(int entry) {
// 	return (entry + 1) % tableSize;
// }

// float BaseHueForPosition(int index) {
// 	return index * (hueLimit / tableSize);
// }


#endif