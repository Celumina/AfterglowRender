#ifndef RANDOM_HLSL
#define RANDOM_HLSL

#include "Common.hlsl"
#include "Constants.hlsl"

static const float randomSeed0 = 1312.21551f;
static const float randomSeed1 = 5441.35424f;
static const float randomSeed2 = 3829.19572f;  
static const float randomSeed3 = 2948.75398f;  
static const float randomSeed4 = 4756.28463f;  
static const float randomSeed5 = 1789.54627f;  
static const float randomSeed6 = 2841.93752f;  
static const float randomSeed7 = 7592.18365f;  

// TOOD: Snorm noises.

int BitHash(int x) {
	x = (x ^ 61) ^ (x >> 16);
	x = x + (x << 3);
	x = x ^ (x >> 4);
	x = x * 0x27d4eb2d;
	x = x ^ (x >> 15);
	return x;
}

float UniformBitHash(int x) {
	return float(BitHash(x)) / float(0x7FFFFFFF);
}

float Hash(float x) {
	return frac(sin(x * 15368.6234 + 475.053) + 15413.21);
}

float Hash(float x, float seed = randomSeed0) {
	return frac(sin(x * 14523.46187) * seed);
}

float Hash(float2 xy, float seed = randomSeed0) {
	return frac(sin(dot(xy, float2(13.6196,54.2197))) * 43758.5453123 + seed);
}

float2 Hash2D(float2 xy, float seed = randomSeed0) {
   return frac(sin(float2(
		dot(xy, float2(46.531, 91.4653)), 
		dot(xy, float2(64.4634, 49.4349))
	)) * 13127.643 + seed);
}

float3 Hash3D(float3 xyz, float seed = randomSeed0) {
	return frac(sin(float3(
		dot(xyz, float3(83.7247, 71.7823, 24.274)), 
		dot(xyz, float3(64.4634, 49.4349, 82.263)), 
		dot(xyz, float3(94.262, 20.9245, 34.8256))
	)) * 52567.0925 + seed);
}

// Usage: Good fit with screen position.
float PseudoRandom(float2 xy) {
	float2 pos = frac(xy / 128.0) * 128.0 + float2(-43.432436, -72.246132);
	return frac(dot(pos.xyx * pos.xyy, float3(29.23412, 71.235082, 4.532124)));
}
 
float InterpolatedNoise(float2 xy, float2 scale, float seed = randomSeed0) {
	float2 scaledXY = xy * scale;
	float2 floorXY = floor(scaledXY) / scale;
	float2 ceilXY = ceil(scaledXY) / scale;

	float hashFF = Hash(floorXY, seed);
	float hashFC = Hash(float2(floorXY.x, ceilXY.y), seed);
	float hashCF = Hash(float2(ceilXY.x, floorXY.y), seed);
	float hashCC = Hash(ceilXY, seed);

	float2 weight = scaledXY - floor(scaledXY);
	return lerp(lerp(hashFF, hashFC, weight.y), lerp(hashCF, hashCC, weight.y), weight.x);
}

float SmoothInterpolatedNoise(float2 xy, float2 scale, float seed = randomSeed0) {
	float2 scaledXY = xy * scale;
	float2 floorXY = floor(scaledXY) / scale;
	float2 ceilXY = ceil(scaledXY) / scale;

	float hashFF = Hash(floorXY, seed);
	float hashFC = Hash(float2(floorXY.x, ceilXY.y), seed);
	float hashCF = Hash(float2(ceilXY.x, floorXY.y), seed);
	float hashCC = Hash(ceilXY, seed);

	float2 weight = smoothstep(0.0, 1.0, scaledXY - floor(scaledXY));
	return lerp(lerp(hashFF, hashFC, weight.y), lerp(hashCF, hashCC, weight.y), weight.x);
}

// TODO: The range of noise seems a bit less than [0, 1]..
float PerlinNoise(float2 xy, float2 scale, float seed = randomSeed0) {
	float2 scaledXY = xy * scale;
	float2 scaledFloorXY = floor(scaledXY);
	float2 scaledCeilXY = ceil(scaledXY);

	float2 floorXY = scaledFloorXY / scale;
	float2 ceilXY = scaledCeilXY / scale;

	float2 gradientFF = Snorm(Hash2D(floorXY, seed));
	float2 gradientFC = Snorm(Hash2D(float2(floorXY.x, ceilXY.y), seed));
	float2 gradientCF = Snorm(Hash2D(float2(ceilXY.x, floorXY.y), seed));
	float2 gradientCC = Snorm(Hash2D(ceilXY, seed));

	float2 distFF = scaledXY - scaledFloorXY;
	float productFF = dot(gradientFF, distFF);
	float productFC = dot(gradientFC, scaledXY - float2(scaledFloorXY.x, scaledCeilXY.y));
	float productCF = dot(gradientCF, scaledXY - float2(scaledCeilXY.x, scaledFloorXY.y));
	float productCC = dot(gradientCC, scaledXY - scaledCeilXY);

	// Interesting mosaic effect.
	// float2 weight = smoothstep(0.5, 1.0, distFF);
	float2 weight = smoothstep(0.0, 1.0, distFF);
	return Unorm(lerp(lerp(productFF, productFC, weight.y), lerp(productCF, productCC, weight.y), weight.x) * sqrt2);
}

float PerlinNoise(float3 xyz, float3 scale, float seed = randomSeed0) {
	float3 scaledXYZ = xyz * scale;
	float3 scaledFloorXYZ = floor(scaledXYZ);
	float3 scaledCeilXYZ = ceil(scaledXYZ);
 
	float3 floorXYZ = scaledFloorXYZ / scale;
	float3 ceilXYZ = scaledCeilXYZ / scale;
 
	float3 gradientFFF = Snorm(Hash3D(floorXYZ, seed));
	float3 gradientFFC = Snorm(Hash3D(float3(floorXYZ.x, floorXYZ.y, ceilXYZ.z), seed));
	float3 gradientFCF = Snorm(Hash3D(float3(floorXYZ.x, ceilXYZ.y, floorXYZ.z), seed));
	float3 gradientFCC = Snorm(Hash3D(float3(floorXYZ.x, ceilXYZ.y, ceilXYZ.z), seed));
	float3 gradientCFF = Snorm(Hash3D(float3(ceilXYZ.x, floorXYZ.y, floorXYZ.z), seed));
	float3 gradientCFC = Snorm(Hash3D(float3(ceilXYZ.x, floorXYZ.y, ceilXYZ.z), seed));
	float3 gradientCCF = Snorm(Hash3D(float3(ceilXYZ.x, ceilXYZ.y, floorXYZ.z), seed));
	float3 gradientCCC = Snorm(Hash3D(ceilXYZ, seed));
 
	float3 distFFF = scaledXYZ - scaledFloorXYZ;
	float productFFF = dot(gradientFFF, distFFF);   
	float productFFC = dot(gradientFFC, scaledXYZ - float3(scaledFloorXYZ.x, scaledFloorXYZ.y, scaledCeilXYZ.z));
	float productFCF = dot(gradientFCF, scaledXYZ - float3(scaledFloorXYZ.x, scaledCeilXYZ.y, scaledFloorXYZ.z));
	float productFCC = dot(gradientFCC, scaledXYZ - float3(scaledFloorXYZ.x, scaledCeilXYZ.y, scaledCeilXYZ.z));
	float productCFF = dot(gradientCFF, scaledXYZ - float3(scaledCeilXYZ.x, scaledFloorXYZ.y, scaledFloorXYZ.z));
	float productCFC = dot(gradientCFC, scaledXYZ - float3(scaledCeilXYZ.x, scaledFloorXYZ.y, scaledCeilXYZ.z));
	float productCCF = dot(gradientCCF, scaledXYZ - float3(scaledCeilXYZ.x, scaledCeilXYZ.y, scaledFloorXYZ.z));
	float productCCC = dot(gradientCCC, scaledXYZ - scaledCeilXYZ);
 
	float3 weight = smoothstep(0.0, 1.0, distFFF); 

	float lerpFF = lerp(
		lerp(productFFF, productFCF, weight.y),
		lerp(productFFC, productFCC, weight.y),
		weight.z
	);
	float lerpCF = lerp(
		lerp(productCFF, productCCF, weight.y),
		lerp(productCFC, productCCC, weight.y),
		weight.z
	);
	return Unorm(lerp(lerpFF, lerpCF, weight.x) * sqrt2);
}

float SimplexNoise(float2 xy, float2 scale, float seed = randomSeed0) {
	float2 scaledXY = xy * scale;

	// 2d skew constants.
	// F: perpendicular triangle to equilateral triangle constants.
	// F = sqrt(dimension) - 1 / dimension;
	static const float F = 0.366025404; // (sqrt(3) - 1) / 2
	static const float G = 0.211324865; // (3 - sqrt(3)) / 6

	// SkewedSpace = uv + (uv.x + uv.y) * F;
	// SkewedID = floor(skewedSpace)
	float2 skewedID = floor(scaledXY + (scaledXY.x + scaledXY.y) * F);
	
	// Vector form skewed to standard space origin of scaleValue tile's(0.0, 0.0) coord.
	float2 skewedToStdOriginDir = scaledXY - (skewedID - (skewedID.x + skewedID.y) * G);
	
	// Half sign, to decide which triangle is skewed point on.
	float2 halfSign = skewedToStdOriginDir.x > skewedToStdOriginDir.y ? float2(1.0, 0.0) : float2(0.0, 1.0);
	
	// Vector form skewed to standard space adjacent of scaleValue tile's (1.0, 0.0) or (0.0, 1.0)
	// Depend on point which is closer to skewed.
	float2 skewedToStdAdjacentDir = skewedToStdOriginDir - halfSign + G;
	
	// Vector form skewed to standard space  scaleValue's (1.0, 1.0)
	float2 skewedToStdOppositeDir = skewedToStdOriginDir - 1.0 + 2.0 * G;

	// barycenter coord, each component means:
	// distance form current skewedpoint to each vertex(A, B, C) inside the current triangle.
	float3 barycenter = {
		dot(skewedToStdOriginDir, skewedToStdOriginDir), 
		dot(skewedToStdAdjacentDir, skewedToStdAdjacentDir), 
		dot(skewedToStdOppositeDir, skewedToStdOppositeDir)
	};

	// Here 0.5 is equilateral triangle's height square.
	// Square because simplex use distance square attenuation as it's weight.
	// Replace by 0.6 gets better result, ref from Ken Perlin.
	float3 invHalfBarycenter = max(0.5 - barycenter, 0.0);
	
	float hashOri = dot(skewedToStdOriginDir, Snorm(Hash2D(skewedID, seed)));
	float hashAdj = dot(skewedToStdAdjacentDir, Snorm(Hash2D(skewedID + halfSign, seed)));
	float hashOpp = dot(skewedToStdOppositeDir, Snorm(Hash2D(skewedID + 1.0, seed)));
	
	// For reduce instructions.
	float3 doubleInvHalfBarycenter = invHalfBarycenter * invHalfBarycenter;
	
	float3 hashGradiant = 
		doubleInvHalfBarycenter * doubleInvHalfBarycenter * float3(hashOri, hashAdj, hashOpp);
	
	// Constant 70.0 makes value to normalized range [-1, 1]
	// It's from: (1 / 3) ^ 4 * (1 / sqrt(6)) * (2 * sqrt(2)) approximate to 1 / 70.
	return Unorm(dot(float3(70.0, 70.0, 70.0), hashGradiant));
}

float SimplexNoise(float3 xyz, float3 scale, float seed = randomSeed0) {
	float3 scaledXYZ = xyz * scale;

	static const float F = 0.333333333; // (sqrt(4) - 1) / 3 = 1/3
	static const float G = 0.166666667; // (1 - 1/sqrt(4)) / 3 = 1/6
 
	float3 skewedID = floor(scaledXYZ + dot(scaledXYZ, float3(F, F, F)));
	float3 skewedToOrigin = scaledXYZ - skewedID + dot(skewedID, float3(G, G, G));
 
	float3 skewedGreaterThanCurrentComp = step(0.0, skewedToOrigin - skewedToOrigin.yzx);
	float3 halfSign0 = skewedGreaterThanCurrentComp * (1.0 - skewedGreaterThanCurrentComp.zxy);
	float3 halfSign1 = 1.0 - skewedGreaterThanCurrentComp.zxy * (1.0 - skewedGreaterThanCurrentComp);

	// Skewed point to tetrahedron Point[n]'s direction.
	float3 skewedToPoint0 = skewedToOrigin - halfSign0 + G;
	float3 skewedToPoint1 = skewedToOrigin - halfSign1 + 2.0 * G;
	float3 skewedToPoint2 = skewedToOrigin - 1.0 + 3.0 * G;

	float4 weight = {
		dot(skewedToOrigin, skewedToOrigin), 
		dot(skewedToPoint0, skewedToPoint0), 
		dot(skewedToPoint1, skewedToPoint1), 
		dot(skewedToPoint2, skewedToPoint2), 
	};
	weight = max(0.5 - weight, 0.0);

	float4 hash = {
		dot(Snorm(Hash3D(skewedID, seed)), skewedToOrigin), 
		dot(Snorm(Hash3D(skewedID + halfSign0, seed)), skewedToPoint0), 
		dot(Snorm(Hash3D(skewedID + halfSign1, seed)), skewedToPoint1), 
		dot(Snorm(Hash3D(skewedID + 1.0, seed)), skewedToPoint2)
	};

	return Unorm(dot(hash * Pow4(weight), float4(52.0, 52.0, 52.0, 52.0)));
}

float WorleyNoise(float2 xy, float2 scale, float seed = randomSeed0) {
	float2 scaledXY = xy * scale;
	float2 scaledFloorXY = floor(scaledXY);
	
	float minDistance = 1.0;
	[unroll] for (int offsetY = -1; offsetY <= 1; ++offsetY) {
		[unroll] for (int offsetX = -1; offsetX <= 1; ++offsetX) {
			float2 tilePos = scaledFloorXY + float2(offsetX, offsetY);
			// Here use ManhattanDistance() to get a different effect.
			float distanceToPoint = distance(scaledXY, float2(offsetX, offsetY) + Hash2D(tilePos, seed) + scaledFloorXY);
			minDistance = min(distanceToPoint, minDistance);
		}
	} 
	return minDistance;
}

float WorleyNoise(float3 xyz, float3 scale, float seed = randomSeed0) {
	float3 scaledXYZ = xyz * scale;
	float3 scaledFloorXYZ = floor(scaledXYZ);
	
	float minDistance = 1.0;
	[unroll] for (int offsetZ = -1; offsetZ <= 1; ++offsetZ) {
		[unroll] for (int offsetY = -1; offsetY <= 1; ++offsetY) {
			[unroll] for (int offsetX = -1; offsetX <= 1; ++offsetX) {
				float3 tilePos = scaledFloorXYZ + float3(offsetX, offsetY, offsetZ);
				// Here use ManhattanDistance() to get a different effect.
				float distanceToPoint = distance(
					scaledXYZ, float3(offsetX, offsetY, offsetZ) + Hash3D(tilePos, seed) + scaledFloorXYZ
				);
				minDistance = min(distanceToPoint, minDistance);
			}
		} 
	}
	return minDistance;
}


#endif