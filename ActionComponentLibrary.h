#pragma once

#include "AfterglowActionComponent.h"
#include "AfterglowTransform.h"

// Action Component Library
namespace acl {
	class EntityRotator;
	class SimpleController;
	class ParticleSpawner;
};


class acl::EntityRotator : public AfterglowActionComponent<EntityRotator> {
public: 
	void update(); 
};


class acl::SimpleController : public AfterglowActionComponent<SimpleController> {
public:
	void update();

private:
	bool _fpvMode = false;
	AfterglowTranslation _currentMoveVelocity = {0.0f, 0.0f, 0.0f};
	AfterglowInput::Position _cursorPosLastUpdate = {0.0, 0.0};
};


class acl::ParticleSpawner : public AfterglowActionComponent<ParticleSpawner> {
public:
	struct Particle {
		glm::vec3 position;
		glm::vec3 velocity;
		glm::vec4 color;
	};
	
	void awake();
	void update();

private:
	std::vector<Particle> _particles;

};