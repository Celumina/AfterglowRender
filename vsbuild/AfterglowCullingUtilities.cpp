#include "AfterglowCullingUtilities.h"

inline bool AABBOutsideFrustumPlane(const glm::vec4& plane, const model::AABB& aabb) {
	/* Find the AABB corner farthest in the direction of the plane's normal */ 
	glm::vec3 farthestCorner = { aabb.min[0], aabb.min[1], aabb.min[2] };
	if (plane.x > 0.0f) {
		farthestCorner.x = aabb.max[0];
	}
	if (plane.y > 0.0f) {
		farthestCorner.y = aabb.max[1];
	}
	if (plane.z > 0.0f) {
		farthestCorner.z = aabb.max[2];
	}

	/* Distance from farthest corner to plane */
	float dist = glm::dot(glm::vec3(plane), farthestCorner) + plane.w;
	/* If farthest corner is outside → AABB is outside */
	return dist < 0.0f;
}

model::AABB util::TransformAABB(const model::AABB& aabb, const glm::mat4& mat) {
	const std::array<glm::vec3, 8> vertices = {
		glm::vec3(aabb.min[0], aabb.min[1], aabb.min[2]),
		glm::vec3(aabb.min[0], aabb.min[1], aabb.max[2]), 
		glm::vec3(aabb.min[0], aabb.max[1], aabb.min[2]), 
		glm::vec3(aabb.min[0], aabb.max[1], aabb.max[2]), 
		glm::vec3(aabb.max[0], aabb.min[1], aabb.min[2]), 
		glm::vec3(aabb.max[0], aabb.min[1], aabb.max[2]), 
		glm::vec3(aabb.max[0], aabb.max[1], aabb.min[2]), 
		glm::vec3(aabb.max[0], aabb.max[1], aabb.max[2])  
	};

	glm::vec3 tranformedMin = glm::vec3(mat * glm::vec4(vertices[0], 1.0f));
	glm::vec3 tranformedMax = tranformedMin;

	for (size_t i = 1; i < vertices.size(); ++i) {
		glm::vec3 transformedPosition = glm::vec3(mat * glm::vec4(vertices[i], 1.0f));

		tranformedMin.x = glm::min(tranformedMin.x, transformedPosition.x);
		tranformedMin.y = glm::min(tranformedMin.y, transformedPosition.y);
		tranformedMin.z = glm::min(tranformedMin.z, transformedPosition.z);

		tranformedMax.x = glm::max(tranformedMax.x, transformedPosition.x);
		tranformedMax.y = glm::max(tranformedMax.y, transformedPosition.y);
		tranformedMax.z = glm::max(tranformedMax.z, transformedPosition.z);
	}

	return model::AABB{
		.min = { tranformedMin.x, tranformedMin.y, tranformedMin.z },
		.max = { tranformedMax.x, tranformedMax.y, tranformedMax.z }
	};
}

model::AABB util::NonshearTransformAABB(const model::AABB& aabb, const glm::mat4& mat) {
	glm::mat3 linearMat = glm::mat3(mat); 
	glm::vec3 translate = glm::vec3(mat[3]); 

	glm::vec3 aabbCenter = glm::vec3{ aabb.min[0] + aabb.max[0], aabb.min[1] + aabb.max[1], aabb.min[2] + aabb.max[2] } * 0.5f;
	glm::vec3 aabbHalfExtent = glm::vec3{ aabb.max[0] - aabb.min[0], aabb.max[1] - aabb.min[1], aabb.max[2] - aabb.min[2]} * 0.5f;

	glm::vec3 transformedHalfExtent{0.0f};
	for (int i = 0; i < 3; ++i) {
		glm::vec3 column = linearMat[i]; 
		transformedHalfExtent.x += glm::abs(column.x) * aabbHalfExtent[i];
		transformedHalfExtent.y += glm::abs(column.y) * aabbHalfExtent[i];
		transformedHalfExtent.z += glm::abs(column.z) * aabbHalfExtent[i];
	}
	glm::vec3 transformedCenter = linearMat * aabbCenter + translate;

	glm::vec3 transformedAABBMin = transformedCenter - transformedHalfExtent;
	glm::vec3 transformedAABBMax = transformedCenter + transformedHalfExtent;
	return model::AABB{ 
		.min = { transformedAABBMin.x, transformedAABBMin.y, transformedAABBMin.z }, 
		.max = { transformedAABBMax.x, transformedAABBMax.y, transformedAABBMax.z },
	};
}

bool util::FrustumCulling(ubo::GlobalUniform& globalUniform, const model::AABB& aabb) {
	if (AABBOutsideFrustumPlane(globalUniform.frustumPlaneL, aabb)) return true;
	if (AABBOutsideFrustumPlane(globalUniform.frustumPlaneR, aabb)) return true;
	if (AABBOutsideFrustumPlane(globalUniform.frustumPlaneB, aabb)) return true;
	if (AABBOutsideFrustumPlane(globalUniform.frustumPlaneT, aabb)) return true;
	if (AABBOutsideFrustumPlane(globalUniform.frustumPlaneN, aabb)) return true;
	if (AABBOutsideFrustumPlane(globalUniform.frustumPlaneF, aabb)) return true;
	return false;
}
