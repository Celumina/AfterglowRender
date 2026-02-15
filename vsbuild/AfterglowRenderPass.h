#pragma once
#include "AfterglowDevice.h"
#include "AfterglowSubpassContext.h"

//#include "AfterglowPassSetBase.h"

/* Pipeline and RenderPass
RenderPass has higher level then Pipeline, one RendePass may contains many pipelines.
- RenderPass define the full Render framework.
- Pipeline define a specified object's render scheme.

Forward Rendering and Deferred Rendering are Frameworks, not Pipeline.
*/

class AfterglowRenderPass : public AfterglowProxyObject<AfterglowRenderPass, VkRenderPass, VkRenderPassCreateInfo> {
public:
	//using Parent = AfterglowProxyObject<AfterglowRenderPass, VkRenderPass, VkRenderPassCreateInfo>;

	AfterglowRenderPass(AfterglowDevice& device);
	~AfterglowRenderPass();
 
	//void initialize();
	inline AfterglowDevice& device() noexcept { return _device; };

	inline AfterglowSubpassContext& subpassContext() noexcept { return _subpassContext; }
	inline const AfterglowSubpassContext& subpassContext() const noexcept { return _subpassContext; }

proxy_protected:
	void initCreateInfo();
	void create();

private:
	AfterglowDevice& _device;
	AfterglowSubpassContext _subpassContext{};
};

