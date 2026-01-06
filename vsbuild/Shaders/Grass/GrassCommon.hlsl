#ifndef GRASS_COMMON_HLSL
#define GRASS_COMMON_HLSL

#include "../WorldDefinitions.hlsl"

static const uint grassDataSideElements = 2048;
static const uint grassVisibleSideElement = 128;

static const float grassVisibleDistance = 160.0;

static const float grassInvDataSideElements = 1.0 / grassDataSideElements;

static const float grassDataInterval = worldSideLength / grassDataSideElements;
static const float grassInvDataInterval = 1.0 / grassDataInterval;


#endif