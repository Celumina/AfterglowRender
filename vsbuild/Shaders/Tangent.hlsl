#ifndef TANGENT_HLSL
#define TANGENT_HLSL

// Position-based tangent. reference: TangentNode, Blender.

float3 TangentPositionAxisX(float3 positionObjectSpace) {
	return positionObjectSpace.xzy * float3(0.0, -0.5, 0.5) + float3(0.0, 0.25, -0.25);
}

float3 TangentPositionAxisY(float3 positionObjectSpace) {
	return positionObjectSpace.zyx * float3(-0.5, 0.0, 0.5) + float3(0.25, 0.0, -0.25);
}

float3 TangentPositionAxisZ(float3 positionObjectSpace) {
	return positionObjectSpace.yxz * float3(-0.5, 0.5, 0.0) + float3(0.25, -0.25, 0.0);
}

// @param tangentPositionWorldSpace: mul(modelMat, float4(TangentPositionAxis(objPos), 1.0)).xyz
float3 RadialTangent(float3 tangentPositionWorldSpace, float3 worldNormal) {
	return cross(worldNormal, normalize(cross(tangentPositionWorldSpace, worldNormal)));
}

#endif