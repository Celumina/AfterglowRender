#pragma once
#include "AfterglowRenderPass.h"
class AfterglowFramebuffer : public AfterglowProxyObject<AfterglowFramebuffer, VkFramebuffer, VkFramebufferCreateInfo> {
public:
	using ImageViewArray = std::vector<VkImageView>;
	AfterglowFramebuffer(AfterglowRenderPass& renderPass);
	~AfterglowFramebuffer();

	// append a attachment info should before create object.
	void appendImageView(VkImageView imageView);

proxy_protected:
	void initCreateInfo();
	void create();

private:
	AfterglowRenderPass& _renderPass;
	std::unique_ptr<ImageViewArray> _imageViewAttachments;
};

