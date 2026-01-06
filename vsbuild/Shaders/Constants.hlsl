#ifndef CONSTANTS_HLSL
#define CONSTANTS_HLSL

static const float pi = 3.14159265359;
static const float invPi = 0.31830988618;
static const float sqrt2 = 1.41421356237;
static const float sqrt3 = 1.73205080756;

static const float inf = asfloat(0x7F800000);
static const float minimumPositive = asfloat(0x00800000); 

static const float3x3 identityMat3x3 = {
	1.0, 0.0, 0.0, 
	0.0, 1.0, 0.0, 
	0.0, 0.0, 1.0
};

#endif