#ifndef COMMON_HLSL
#define COMMON_HLSL

#include "Constants.hlsl"
#include "Chromatics/ACESCommon.hlsl"

template<typename Type>
Type Snorm(Type unromVal) {
	return unromVal * 2.0 - 1.0;
}

template<typename Type>
Type Unorm(Type snormVal) {
	return snormVal * 0.5 + 0.5;
}

template<typename Type>
Type EncodeNormal(Type normal) {
	return Unorm(normal);
}

template<typename Type>
Type DecodeNormal(Type normal) {
	return Snorm(normal);
}

float3 ScaleNormal(float3 normal, float intensity) {
	return normalize(float3(normal.xy * intensity, normal.z));
}

float3 ReconstructNormal(float2 normalXY) {
	return float3(normalXY, sqrt(max(1.0 - (normalXY.x * normalXY.x + normalXY.y * normalXY.y), 0.0)));
}

float3 LoadNormalTexture(Texture2D normalTex, SamplerState texSampler, float2 uv) {
	return DecodeNormal(normalTex.Sample(texSampler, uv).xyz);
}

float3 LoadNormalTextureFromXY(Texture2D normalTex, SamplerState texSampler, float2 uv) {
	return ReconstructNormal(DecodeNormal(normalTex.Sample(texSampler, uv).xy));
}

// Normal TangentSpace -> WorldSpace
float3 NormalMap(float3 normalTS, float3x3 tbn, bool isFrontFace) {
	float3 tangent = isFrontFace ? tbn[0] : -tbn[0];
	return normalTS.x * tangent.xyz + normalTS.y * tbn[1] + normalTS.z * tbn[2];
}

float3 BlendAngleCorrectedNormals(float3 baseNormal, float3 additionalNormal) {
	float3 n0 = float3(baseNormal.xy, baseNormal.z + 1.0);
	float3 n1 = float3(-additionalNormal.xy, additionalNormal.z);
	return normalize(dot(n0, n1) * n0 - n0.z * n1);
}

// TODO: Use shared sampler instead.
float2 MirrorTexCoord(float2 texCoord) {
	return abs((texCoord - 1.0) % 2.0 - 1.0);
}

// @return: vector which is projected on the plane.
float3 ProjectVector(float3 vec, float3 planeNormal) {
	planeNormal = normalize(planeNormal);
	return vec - dot(vec, planeNormal) * planeNormal;
}

// @return: direction(normalized) which is projected on the plane.
float3 ProjectDirection(float3 vec, float3 planeNormal) {
	return normalize(vec - dot(vec, planeNormal) * planeNormal);
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

template<typename Type>
Type ClampVectorWithLength(Type v, float maxLen) {
	float sqrLenV = dot(v, v);
	float invLenV = rsqrt(sqrLenV);
	return v * min(invLenV * maxLen, 1.0);
}

template<typename Type>
Type ClampVectorWithLength(Type v, float minLen, float maxLen) {
	float sqrLenV = dot(v, v);
	float invLenV = rsqrt(sqrLenV);
	return v * max(min(invLenV * maxLen, 1.0), invLenV * minLen);
}

// Rendering working space: AP1 (ACEScg)
float3 LuminanceFactors() {
	return AP1ToXYZMat._m10_m11_m12;
}

float Luminance(float3 linearColor) {
	return dot(linearColor, LuminanceFactors());
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

float2 Rotate(float2 direction, float theta) {
	float sinTheta;
	float cosTheta;
	sincos(theta, sinTheta, cosTheta);
	return mul(
		float2x2(cosTheta, -sinTheta, sinTheta, cosTheta), 
		direction
	);
}

float SafePow(float base, float exponent) { 
	return (base < 0.0 && exponent != floor(exponent)) ? 0.0 : pow(base, exponent);
}

float2 SafePow(float2 base, float2 exponent) {
	return float2(
		SafePow(base[0], exponent[0]), 
		SafePow(base[1], exponent[1])
	);
}

float3 SafePow(float3 base, float3 exponent) {
	return float3(
		SafePow(base[0], exponent[0]), 
		SafePow(base[1], exponent[1]), 
		SafePow(base[2], exponent[2])
	);
}

float4 SafePow(float4 base, float4 exponent) {
	return float4(
		SafePow(base[0], exponent[0]), 
		SafePow(base[1], exponent[1]), 
		SafePow(base[2], exponent[2]), 
		SafePow(base[3], exponent[3])
	);
}

template<typename Type>
Type SafeDivide(Type a, Type b) {
	if (b == 0.0) {
		return 0.0;
	}
	else {
		return a / b;
	}
}

// @return: x with y's sign.
template<typename Type>
Type CopySign(Type x, Type y) {
	return sign(y) * abs(x);
}

template<typename Type>
Type WrapTo360(Type x) {
	float y = fmod(x, 360.0);
	if (y < 0.0) {
		y += 360;
	}
	return y;
}

template<typename Type>
Type MidPoint(Type a, Type b) {
	return (a + b) * 0.5;
}

// Matrix methods
float3x3 ScaleMatrixDiagonal(float3x3 a, float3 v) {
	float3x3 b = a;
	b[0][0] = a[0][0] * v[0];
	b[1][1] = a[1][1] * v[1];
	b[2][2] = a[2][2] * v[2];
	return b;
}

float3x3 Float4x3ToFloat3x3(float4x3 mat) {
	return float3x3(mat[0].xyz, mat[1].xyz, mat[2].xyz);
}

// Calculate determinant of a 2x2 matrix (columns: a, b; rows: x, y)
float Determinant2x2(float ax, float ay, float bx, float by) {
	return ax * by - ay * bx;
}

float3x3 Inverse(float3x3 m, float epsilon = 1e-8f)
{
	// Step 1: Calculate the determinant of the 3x3 matrix
	float det = 
		m[0][0] * Determinant2x2(m[1][1], m[1][2], m[2][1], m[2][2]) -
		m[1][0] * Determinant2x2(m[0][1], m[0][2], m[2][1], m[2][2]) +
		m[2][0] * Determinant2x2(m[0][1], m[0][2], m[1][1], m[1][2]);

	// Guard against singular matrix (divide by zero)
	if (abs(det) < epsilon) {
		return identityMat3x3;
	}
	float invDet = 1.0f / det;

	// Step 2: Calculate the cofactor matrix (column-major)
	float3x3 cofactor;
	// Column 0
	cofactor[0][0] =  Determinant2x2(m[1][1], m[1][2], m[2][1], m[2][2]);
	cofactor[0][1] = -Determinant2x2(m[1][0], m[1][2], m[2][0], m[2][2]);
	cofactor[0][2] =  Determinant2x2(m[1][0], m[1][1], m[2][0], m[2][1]);
	// Column 1
	cofactor[1][0] = -Determinant2x2(m[0][1], m[0][2], m[2][1], m[2][2]);
	cofactor[1][1] =  Determinant2x2(m[0][0], m[0][2], m[2][0], m[2][2]);
	cofactor[1][2] = -Determinant2x2(m[0][0], m[0][1], m[2][0], m[2][1]);
	// Column 2
	cofactor[2][0] =  Determinant2x2(m[0][1], m[0][2], m[1][1], m[1][2]);
	cofactor[2][1] = -Determinant2x2(m[0][0], m[0][2], m[1][0], m[1][2]);
	cofactor[2][2] =  Determinant2x2(m[0][0], m[0][1], m[1][0], m[1][1]);

	// Step 3: Adjugate matrix = transpose of cofactor matrix
	float3x3 adjugate = float3x3(
		cofactor[0][0], cofactor[1][0], cofactor[2][0], // Row 0 = cofactor col 0
		cofactor[0][1], cofactor[1][1], cofactor[2][1], // Row 1 = cofactor col 1
		cofactor[0][2], cofactor[1][2], cofactor[2][2]  // Row 2 = cofactor col 2
	);

	// Step 4: Inverse = adjugate * inverse determinant
	return adjugate * invDet;
}

#endif