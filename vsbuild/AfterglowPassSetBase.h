#pragma once

#include <vector>
#include <memory>
#include "AfterglowObject.h"
#include "AfterglowPassInterface.h"

class AfterglowDrawCommandBuffer;

namespace render {
	template<typename Type>
	concept PassSetType = requires {
		typename Type::PassSetTag;
	};
}

// AfterglowPassSetBase for dynamic polymorphic
class AfterglowPassSetBase :  public AfterglowObject {
public:
	// Passes will be excuted in order by submitCommands().
	using Passes = std::vector<std::unique_ptr<AfterglowPassInterface>>;

	inline Passes& passes() noexcept { return _passes; }

	virtual void submitCommands(AfterglowDrawCommandBuffer& drawCommandBuffer) = 0;

private: 
	Passes _passes;
};
