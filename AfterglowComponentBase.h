#pragma once

#include <stdint.h>
#include <typeindex>

#include "AfterglowObject.h"
#include "Inreflect.h"

class AfterglowComponentBase : AfterglowObject {
public:
	using ID = uint64_t;

	AfterglowComponentBase();
	ID id() const;

	// @brief: true if component just actual one (has same address with "other").
	bool operator==(const AfterglowComponentBase& other) const;

protected:
	ID _id;

private:
	static inline ID _allocatedID = 0;
};

INR_CLASS(AfterglowComponentBase) {
	INR_FUNCS(
		INR_FUNC(id)
	);
};
