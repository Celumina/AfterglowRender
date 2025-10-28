#pragma once

#include "AfterglowActionComponent.h"
#include "AfterglowTransform.h"

// Action Component Library
namespace acl {
	class EntityRotator;
	class SimpleController;
	class ParticleSpawner; // TODO: Remove it 
};


class acl::EntityRotator : public AfterglowActionComponent<EntityRotator> {
public: 
	void update(); 

	float angularSpeed() const noexcept;
	void setAngularSpeed(float speed) noexcept;

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
	void update();

private:
	bool _fpvMode = false;
	AfterglowTranslation _currentMoveVelocity = {0.0f, 0.0f, 0.0f};
	glm::dvec2 _cursorPosLastUpdate = {0.0, 0.0};
};


INR_CLASS(acl::SimpleController) {
	INR_BASE_CLASSES<AfterglowActionComponent<acl::SimpleController>>;
};
