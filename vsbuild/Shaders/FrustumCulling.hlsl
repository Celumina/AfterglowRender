#ifndef FRUSTUM_CULLING_HLSL
#define FRUSTUM_CULLING_HLSL

// frustumPlane.xyz: normal; frustumPlane.w: distance.

#define __CHECK_FRUSTUM_PLANES(CHECK_MARCO) \
	CHECK_MARCO(frustumPlaneL); \
	CHECK_MARCO(frustumPlaneR); \
	CHECK_MARCO(frustumPlaneB); \
	CHECK_MARCO(frustumPlaneT); \
	CHECK_MARCO(frustumPlaneN); \
	CHECK_MARCO(frustumPlaneF); \

#define __CHECK_AABB_OUTSIDE_FRUSTUM_PLANE(plane) \
{ \
	/* Find the AABB corner farthest in the direction of the plane's normal */ \
	float3 farthestCorner = aabbMin; \
	if (plane.x > 0.0) { \
		farthestCorner.x = aabbMax.x; \
	} \
	if (plane.y > 0.0) { \
		farthestCorner.y = aabbMax.y; \
	} \
	if (plane.z > 0.0) { \
		farthestCorner.z = aabbMax.z; \
	} \
	\
	/* Distance from farthest corner to plane */  \
	float dist = dot(plane.xyz, farthestCorner) + plane.w; \
	/* If farthest corner is outside → AABB is outside */  \
	if (dist < 0.0) { \
		return true; \
	} \
}

bool IsAABBOutsideFrustum(float3 aabbMin, float3 aabbMax) {
	__CHECK_FRUSTUM_PLANES(__CHECK_AABB_OUTSIDE_FRUSTUM_PLANE);
	return false;
}

#define __CHECK_SPHERE_OUTSIDE_FRUSTUM_PLANE(plane) \
{ \
	/* Distance from sphere center to plane: dot(normal, center) + distance */ \
	float distToPlane = dot(plane.xyz, center) + plane.w; \
	/* If sphere is fully outside the plane -> cull */ \
	if (distToPlane < -radius - 1e-6f) { \
		return true; \
	} \
}

bool IsSphereOutsideFrustum(float3 center, float radius) {
	__CHECK_FRUSTUM_PLANES(__CHECK_SPHERE_OUTSIDE_FRUSTUM_PLANE);
	return false;
}

#endif