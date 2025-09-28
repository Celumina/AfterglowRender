#include "AfterglowComponentBase.h"

AfterglowComponentBase::AfterglowComponentBase() :
	_id(++_allocatedID) {
}

AfterglowComponentBase::ID AfterglowComponentBase::id() const {
	return _id;
}

bool AfterglowComponentBase::operator==(const AfterglowComponentBase& other) const {
	return this == &other;
}
