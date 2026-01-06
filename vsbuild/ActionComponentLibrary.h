#pragma once

#include "AfterglowActionComponent.h"
#include "AfterglowTransform.h"

// Action Component Library
namespace acl {
	class EntityRotator;
	class SimpleController;
	class InteractiveTest; 
};


class acl::EntityRotator : public AfterglowActionComponent<EntityRotator> {
public: 
	void update(); 

	inline float angularSpeed() const noexcept { return _angularSpeed; }
	inline void setAngularSpeed(float speed) noexcept { _angularSpeed = speed; }

private:
	float _angularSpeed = 1.0f;
};


INR_CLASS(acl::EntityRotator) {
	INR_BASE_CLASSES<AfterglowActionComponent<acl::EntityRotator>>;
	INR_FUNCS(
		INR_FUNC(angularSpeed), 
		INR_FUNC(setAngularSpeed)
	);
};


class acl::SimpleController : public AfterglowActionComponent<SimpleController> {
public:
	void awake();
	void update();

	inline float movementSpeed() const noexcept { return _movementSpeed; }
	inline void setMovementSpeed(float movementSpeed) noexcept { _movementSpeed = movementSpeed; }

	inline float rotationSpeed() const noexcept { return _rotationSpeed; }
	inline void setRotationSpeed(float rotationSpeed) noexcept { _rotationSpeed = rotationSpeed; }

	inline float zoomingSpeed() const noexcept { return _zoomingSpeed; }
	inline void setZoomingSpeed(float zoomingSpeed) noexcept { _zoomingSpeed = zoomingSpeed; }

private:
	AfterglowTranslation _currentMoveVelocity = {0.0f, 0.0f, 0.0f};
	float _movementSpeed = 2.0f;
	float _movementSpeedMin = 0.5f;
	float _movementSpeedMax = 10.0f;

	glm::dvec2 _cursorPosLastUpdate = {0.0, 0.0};
	float _rotationSpeed = 0.01f;

	float _fov = 45_deg;
	float _fovMin = 1_deg;
	float _fovMax = 120_deg;
	float _zoomingSpeed = 2.0;
	float _currentZoomingSpeed = 0.0f;
	bool _fpvMode = false;
};


INR_CLASS(acl::SimpleController) {
	INR_BASE_CLASSES<AfterglowActionComponent<acl::SimpleController>>;
	INR_FUNCS (
		INR_FUNC(movementSpeed), 
		INR_FUNC(setMovementSpeed), 
		INR_FUNC(rotationSpeed), 
		INR_FUNC(setRotationSpeed), 
		INR_FUNC(zoomingSpeed), 
		INR_FUNC(setZoomingSpeed)
	);
};

class acl::InteractiveTest : public AfterglowActionComponent<acl::InteractiveTest> {
public:
	void awake();
	void update();

private:
	std::string debugViewMaterialName;

};

INR_CLASS(acl::InteractiveTest) {
	INR_BASE_CLASSES<AfterglowActionComponent<acl::InteractiveTest>>;
};