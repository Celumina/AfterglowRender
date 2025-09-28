#include "AfterglowEntity.h"

AfterglowEntity::AfterglowEntity() :	
	_id(0), _parent(nullptr) {
}

AfterglowEntity::ID AfterglowEntity::id() const {
	return _id;
}

const std::string& AfterglowEntity::name() const {
	return _name;
}

AfterglowEntity* AfterglowEntity::parent() {
	return _parent;
}

const AfterglowEntity* AfterglowEntity::parent() const {
	return _parent;
}

AfterglowComponentBase* AfterglowEntity::component(std::type_index typeIndex) {
	if (_components.find(typeIndex) != _components.end()) {
		return _components[typeIndex];
	}
	return nullptr;
}

bool AfterglowEntity::initID(ID id) {
	if (_id == 0) {
		_id = id;
		return true;
	}
	return false;
}

void AfterglowEntity::rename(const std::string& name) {
	_name = name;
}

void AfterglowEntity::setParent(AfterglowEntity& parent) {
	_parent = &parent;
}
