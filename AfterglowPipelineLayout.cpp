#include "AfterglowPipelineLayout.h"

AfterglowPipelineLayout::AfterglowPipelineLayout(AfterglowDevice& device) : 
	_device(device) {
}

AfterglowPipelineLayout::~AfterglowPipelineLayout() {
	destroy(vkDestroyPipelineLayout, _device, data(), nullptr);
}

void AfterglowPipelineLayout::initCreateInfo() {
	info().sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	// info().pSetLayouts set by AfterglowPipeline.
	info().pushConstantRangeCount = 0;
	info().pPushConstantRanges = nullptr;
}

void AfterglowPipelineLayout::create() {
	if (vkCreatePipelineLayout(_device, &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create pipeline layout.");
	}

	// Release _descriptorSetLayout is unnecessary.
	// Because pipeline layout always release in pipeline create function.
}
