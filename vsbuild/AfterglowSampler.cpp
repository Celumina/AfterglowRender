#include "AfterglowSampler.h"
#include "AfterglowPhysicalDevice.h"

AfterglowSampler::AfterglowSampler(AfterglowDevice& device) : 
	_device(device) {
}

AfterglowSampler::~AfterglowSampler() {
	destroy(vkDestroySampler, _device, data(), nullptr);
}

void AfterglowSampler::setAddressModes(VkSamplerAddressMode mode) noexcept {
	if (!isDataExists()) {
		info().addressModeU = mode;
		info().addressModeV = mode;
		info().addressModeW = mode;
	}
	else {
		throw runtimeError("Unable to set address mode, due to vkSampler was created. ");
	}
}

void AfterglowSampler::initCreateInfo() {
	info().sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	// VK_FILTER_CUBIC_IMG is better, but more expensive.
	info().magFilter = VK_FILTER_LINEAR;
	info().minFilter = VK_FILTER_LINEAR;

	info().addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info().addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info().addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	info().anisotropyEnable = VK_TRUE;
	info().maxAnisotropy = _device.physicalDevice().properties().limits.maxSamplerAnisotropy;

	info().borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	info().unnormalizedCoordinates = VK_FALSE;

	info().compareEnable = VK_FALSE;
	info().compareOp = VK_COMPARE_OP_ALWAYS;

	info().mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info().minLod = 0.0f;
	info().minLod = 0.0f;
	info().maxLod = 0.0f;
}

void AfterglowSampler::create() {
	if (vkCreateSampler(_device, &info(), nullptr, &data()) != VK_SUCCESS) {
		throw runtimeError("Failed to create texture sampler.");
	}
}
