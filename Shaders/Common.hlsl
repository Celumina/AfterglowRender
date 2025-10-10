#pragma once

template<typename Type>
Type Snorm(Type unromVal) {
	return unromVal * 2.0 - 1.0;
}

template<typename Type>
Type Unorm(Type snormVal) {
	return snormVal * 0.5 + 0.5;
}

float3 EncodeNormal(float3 normal) {
	return Unorm(normal);
}

float3 DecodeNormal(float3 normal) {
	return Snorm(normal);
}

template<typename Type>
Type Square(Type x) {
	return x * x;
}

template<typename Type>
Type Pow3(Type x) {
	return x * x * x;
}

template<typename Type>
Type Pow4(Type x) {
	Type xx = x * x;
	return xx * xx;
}

template<typename Type>
Type Pow5(Type x) {
	Type xx = x * x;
	return xx * xx * x;
}

float ManhattanDistance(float2 a, float2 b) {
	float2 v = a - b;
	return abs(v.x) + abs(v.y);
}

float ManhattanDistance(float3 a, float3 b) {
	float3 v = a - b;
	return abs(v.x) + abs(v.y) + abs(v.z);
}

float ManhattanDistance(float4 a, float4 b) {
	float4 v = a - b;
	return abs(v.x) + abs(v.y) + abs(v.z) + abs(v.w);
}

float4x4 InverseRigidTransform(float4x4 m) {
    float3x3 rotation = (float3x3) m;
    float3 translation = m[3].xyz;
 
    float3x3 invRotation = transpose(rotation);
    float3 invTranslation = -mul(invRotation, translation);
 
    float4x4 invMat = float4x4(
        float4(invRotation[0], 0.0f),
        float4(invRotation[1], 0.0f),
        float4(invRotation[2], 0.0f),
        float4(invTranslation, 1.0f)
    );
    return invMat;
}