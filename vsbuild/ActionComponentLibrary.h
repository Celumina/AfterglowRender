#pragma once

#include "AfterglowActionComponent.h"
#include "AfterglowTransform.h"

// Action Component Library
namespace acl {
	class EntityRotator;
	class SimpleController;
	class InteractiveTest; 
	class MaterialObjectStateParamUpdater;
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

	inline float movementAcceleration() const noexcept { return _movementAcceleration; }
	inline void setMovementAcceleration(float movementAceeleration) noexcept { _movementAcceleration = movementAceeleration; }

	inline float rotationSpeed() const noexcept { return _rotationSpeed; }
	inline void setRotationSpeed(float rotationSpeed) noexcept { _rotationSpeed = rotationSpeed; }

	inline float rotationAcceleration() const noexcept { return _rotationAcceleration; }
	inline void setRotationAcceleration(float rotationAcceleration) noexcept { _rotationAcceleration = rotationAcceleration; }

	inline float zoomingSpeed() const noexcept { return _zoomingSpeed; }
	inline void setZoomingSpeed(float zoomingSpeed) noexcept { _zoomingSpeed = zoomingSpeed; }

private:
	AfterglowTranslation _currentMoveVelocity = {0.0f, 0.0f, 0.0f};
	float _movementSpeed = 2.0f;
	float _movementSpeedMin = 0.5f;
	float _movementSpeedMax = 10.0f;
	float _movementAcceleration = 4.0f;

	glm::dvec2 _cursorPosLastUpdate = {0.0, 0.0};
	float _rotationSpeed = 0.01f;
	float _rotationAcceleration = 8.0f;
	glm::vec2 _currentRotationVelocity = { 0.0f, 0.0f };

	float _fov = 45_deg;
	float _fovMin = 1_deg;
	float _fovMax = 120_deg;
	float _zoomingSpeed = 2.0f;
	float _currentZoomingSpeed = 0.0f;
	bool _fpvMode = false;
};


INR_CLASS(acl::SimpleController) {
	INR_BASE_CLASSES<AfterglowActionComponent<acl::SimpleController>>;
	INR_FUNCS (
		INR_FUNC(movementSpeed), 
		INR_FUNC(setMovementSpeed), 
		INR_FUNC(movementAcceleration), 
		INR_FUNC(setMovementAcceleration), 
		INR_FUNC(rotationSpeed), 
		INR_FUNC(setRotationSpeed), 
		INR_FUNC(rotationAcceleration), 
		INR_FUNC(setRotationAcceleration), 
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


class acl::MaterialObjectStateParamUpdater : public AfterglowActionComponent<acl::MaterialObjectStateParamUpdater> {
public:
	/**
	* Object state params:
	*	objectForward
	*	objectRight
	*	objectUp
	*	...
	*/

	/**
	* @brief: The updater would append object state params into target material. 
	* @note: One updater can only bind one material, rebind another material would clear all material instance bindings from last material.
	*/
	void bindMaterial(const std::string& name);
	/**
	* @brief: Bind mateiral instance which want to receive object state params every frame.
	* @note: Several mateiral instances can be binded on the same udpater as long as them all belong to the binded material.
	*/
	void bindMaterialInstance(const std::string& name);

	void update();

private:
	void initializeMaterialParams();

	std::string _materialName;
	std::vector<std::string> _materialInstanceNames;
};

INR_CLASS(acl::MaterialObjectStateParamUpdater) {
	INR_BASE_CLASSES<AfterglowActionComponent<acl::MaterialObjectStateParamUpdater>>;
	INR_FUNCS(
		INR_FUNC(bindMaterial), 
		INR_FUNC(bindMaterialInstance)
	);
};