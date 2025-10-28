#pragma once
#include "AfterglowSwapchain.h"
#include "AfterglowSubpassContext.h"

/* Pipeline and RenderPass
RenderPass has higher level then Pipeline, one RendePass may contains many pipelines.
- RenderPass define the full Render framework.
- Pipeline define a specified object's render scheme.

Forward Rendering and Deferred Rendering are Frameworks, not Pipeline.
*/

class AfterglowRenderPass : public AfterglowProxyObject<AfterglowRenderPass, VkRenderPass, VkRenderPassCreateInfo> {
public:
	using AttachmentDescriptionArray = std::vector<VkAttachmentDescription>;
	using AttachmentReferenceArray = std::vector<VkAttachmentReference>;

	AfterglowRenderPass(AfterglowSwapchain& swapchain);
	~AfterglowRenderPass();
 
	inline AfterglowDevice& device() noexcept;
	AfterglowSwapchain& swapchain();

	AfterglowSubpassContext& subpassContext();

proxy_protected:
	void initCreateInfo();
	void create();

private:
	std::unique_ptr<AfterglowSubpassContext> _subpassContext;
	AfterglowSwapchain& _swapchain;
};

