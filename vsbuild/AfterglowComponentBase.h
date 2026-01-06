#pragma once

#include <stdint.h>
#include <typeindex>

#include "AfterglowObject.h"
#include "Inreflect.h"

class AfterglowComponentBase : AfterglowObject {
public:
	using ID = uint64_t;
	static constexpr ID invalidID();

	AfterglowComponentBase();
	ID id() const noexcept;

	// @brief: true if component just actual one (has same address with "other").
	bool operator==(const AfterglowComponentBase& other) const noexcept;

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
