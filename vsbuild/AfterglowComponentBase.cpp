#include "AfterglowComponentBase.h"

constexpr AfterglowComponentBase::ID AfterglowComponentBase::invalidID() {
	return 0;
}

AfterglowComponentBase::AfterglowComponentBase() :
	_id(++_allocatedID) {
}

AfterglowComponentBase::ID AfterglowComponentBase::id() const noexcept {
	return _id;
}

bool AfterglowComponentBase::operator==(const AfterglowComponentBase& other) const noexcept {
	return this == &other;
}
