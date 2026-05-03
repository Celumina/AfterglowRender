#ifndef HAIR_COMMON_HLSL
#define HAIR_COMMON_HLSL

// @params xoh: xoh or yoh, depend on the target uv.
float DistributionKajiyaKay(float xoh, float exponent) {
	return pow(sqrt(1.0 - min(xoh * xoh, 1.0)), exponent);
}

// @brief: Use for Hair strand shift from texture.
float ShiftTangent(float3 tangent, float3 normal, float value) {
	return normalize(tangent + normal * value);
}

// @brief: Based on kajiya kay model with some optimizations.
// @usage: Combine two distribution function for hair. 
// 		The first distribution represents primary specular;
// 		The second distribution represents seconary colorful specular and shifted towards hair root;
float DistributionMarschnerApprox(float xoh, float exponent) {
	// cos^2(x) + sin^2(x) = 1;
	float sinTH = sqrt(1.0 - xoh * xoh);
	float directionAttenuation = smoothstep(-1.0, 0.0, xoh);
	return directionAttenuation * pow(sinTH, exponent);
}

#endif