#pragma once


#include "AfterglowPassSetBase.h"
#include "RenderDefinitions.h"
#include "AfterglowDrawCommandBuffer.h"

// AfterglowPassSet for type info.
// @Domain before the custom subpass set.
// Use template to enforce derived class declare begin domain.
template<render::Domain beginDomainParam>
class AfterglowPassSet : public AfterglowPassSetBase {
public:
	using PassSetTag = void;
	static constexpr render::Domain beginDomain = beginDomainParam;
};

