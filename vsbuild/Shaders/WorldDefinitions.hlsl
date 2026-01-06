#ifndef WORLD_DEFINITIONS_HLSL
#define WORLD_DEFINITIONS_HLSL

static const float worldSideLength = 2048.0;

// TODO: Use a function to handle different gravity in the world.
static const float worldGravity = 9.8; // m/(s^2)

static const float worldInvSideLength = 1.0 / worldSideLength;
static const float worldCenterOffset = worldSideLength * -0.5;

// For data array which covers the whole world.
float2 WorldPositionFromTileID(uint2 id, float invSideElements) {
	return worldSideLength * (invSideElements * id) + worldCenterOffset;
}

uint2 TileIDFromWorldPosition(float2 worldPos, float invInterval) {
	return uint2(clamp((worldPos - worldCenterOffset) * invInterval, 0, worldSideLength * invInterval));
}

#endif