#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

//struct alignas(4) AfterglowTranslation {
//	float x = 0.0;
//	float y = 0.0;
//	float z = 0.0;
//
//	operator glm::vec3() const;
//};
//
//struct alignas(4) AfterglowEuler {
//	float roll = 0.0;
//	float pitch = 0.0;
//	float yaw = 0.0;
//
//	operator glm::vec3() const;
//};
//
//struct alignas(4) AfterglowScaling {
//	float x = 1.0;
//	float y = 1.0;
//	float z = 1.0;
//
//	operator glm::vec3() const;
//};
//
//struct alignas(4) AfterglowQuaternion {
//	float x = 0.0;
//	float y = 0.0;
//	float z = 0.0;
//	float w = 1.0;
//
//	operator glm::quat() const;
//};

using AfterglowDirection = glm::vec3; 

using AfterglowTranslation = glm::vec3;
using AfterglowQuaternion = glm::quat;
using AfterglowScaling = glm::vec3;

using AfterglowEuler = glm::vec3;

struct AfterglowTransform {
	static constexpr AfterglowTranslation defaultTranslation();
	static constexpr AfterglowQuaternion defaultRotation();
	static constexpr AfterglowScaling defaultScaling();

	bool operator==(const AfterglowTransform& other);

	AfterglowTranslation translation = {0.0f, 0.0f, 0.0f};
	AfterglowQuaternion rotation = {1.0f,  0.0f, 0.0f, 0.0f };
	AfterglowScaling scaling = {1.0f, 1.0f, 1.0f};
};

namespace cardinal {
	static constexpr AfterglowDirection Forward = { 0.0f, 1.0f, 0.0f };
	static constexpr AfterglowDirection Backward = { 0.0f, -1.0f, 0.0f };
	static constexpr AfterglowDirection Up = { 0.0f, 0.0f, 1.0f };
	static constexpr AfterglowDirection Down = { 0.0f, 0.0f, -1.0f };
	static constexpr AfterglowDirection Left = { -1.0f, 0.0f, 0.0f };
	static constexpr AfterglowDirection Right = { 1.0f, 0.0f, 0.0f };
}

constexpr AfterglowTranslation AfterglowTransform::defaultTranslation() {
	return { 0.0f, 0.0f, 0.0f };
}

constexpr AfterglowQuaternion AfterglowTransform::defaultRotation() {
	return { 1.0f,  0.0f, 0.0f, 0.0f };
}

constexpr AfterglowScaling AfterglowTransform::defaultScaling() {
	return { 1.0f, 1.0f, 1.0f };
}

inline bool AfterglowTransform::operator==(const AfterglowTransform& other) {
	return translation == other.translation && rotation == other.rotation && scaling == other.scaling;
}
