#pragma once

#include "UniformBufferObjects.h"
#include "AssetDefinitions.h"

namespace util {
	model::AABB TransformAABB(const model::AABB& aabb, const glm::mat4& mat);
	/**
	* @brief: Optimize for non-shear transform. supports rotation, scaling and translation.
	*/
	model::AABB NonshearTransformAABB(const model::AABB& aabb, const glm::mat4& mat);

	/**
	* param globalUniform: for frustum plane info.
	* param aabb: world space Axis aligned bounding box.
	* return: Success to cull.
	*/
	bool FrustumCulling(ubo::GlobalUniform& globalUniform, const model::AABB& aabb);
}