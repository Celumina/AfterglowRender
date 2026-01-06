#pragma once
#include "AfterglowProxyArray.h"
#include "AfterglowPipeline.h"
#include "AfterglowCommandPool.h"
#include "AfterglowDescriptorSetReferences.h"
#include "Configurations.h"

// Because we need a continious command buffer array, so never make this class as a proxy object.
// This Class manage several command buffer that are depend on the settings.
template<typename DerivedType>
class AfterglowCommandBuffer : public AfterglowProxyArray<DerivedType, VkCommandBuffer, VkCommandBufferAllocateInfo> {
public:
	using Parent = AfterglowProxyArray<DerivedType, VkCommandBuffer, VkCommandBufferAllocateInfo>;

	AfterglowCommandBuffer(AfterglowCommandPool& commandPool);
	~AfterglowCommandBuffer();

	// @brief: Current frame command buffer.
	VkCommandBuffer& current() noexcept;
	//uint32_t count();

	void reset(uint32_t currentFrameIndex);

proxy_base_protected(Parent) :
	void initCreateInfo();
	void create();

	void updateCurrentCommandBuffer();
	VkCommandBuffer _currentCommandBuffer = nullptr;

private:
	AfterglowCommandPool& _commandPool;
};


template<typename DerivedType>
inline AfterglowCommandBuffer<DerivedType>::AfterglowCommandBuffer(AfterglowCommandPool& commandPool) :
	Parent(cfg::maxFrameInFlight),
	_commandPool(commandPool) {
	Parent::initialize();
}

template<typename DerivedType>
inline AfterglowCommandBuffer<DerivedType>::~AfterglowCommandBuffer() {
	// Althrough memery free automatically when command pool is destroy, 
	// But here we stiil assume that command buffer has different life period with command pool, so free it manually.
	Parent::destroy(vkFreeCommandBuffers, _commandPool.device(), _commandPool, Parent::size(), Parent::data());
}

template<typename DerivedType>
inline VkCommandBuffer& AfterglowCommandBuffer<DerivedType>::current() noexcept {
	return _currentCommandBuffer;
}

//template<typename DerivedType>
//inline uint32_t AfterglowCommandBuffer<DerivedType>::count() {
//	return sizeof(AfterglowCommandBuffer::Raw) / 0x8;
//}

template<typename DerivedType>
inline void AfterglowCommandBuffer<DerivedType>::reset(uint32_t currentFrameIndex) {
	vkResetCommandBuffer(Parent::data()[currentFrameIndex], 0);
}

template<typename DerivedType>
inline void AfterglowCommandBuffer<DerivedType>::initCreateInfo() {
	Parent::info().sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	Parent::info().level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	Parent::info().commandPool = _commandPool;
	Parent::info().commandBufferCount = cfg::maxFrameInFlight;
}

template<typename DerivedType>
inline void AfterglowCommandBuffer<DerivedType>::create() {
	// It will allocate corresponded count of command buffers according to the allocateInfo. 
	if (vkAllocateCommandBuffers(_commandPool.device(), &Parent::info(), Parent::data()) != VK_SUCCESS) {
		throw Parent::runtimeError("Failed to allocate command buffers.");
	}
}

template<typename DerivedType>
inline void AfterglowCommandBuffer<DerivedType>::updateCurrentCommandBuffer() {
	_currentCommandBuffer = Parent::data()[_commandPool.device().currentFrameIndex()];
}
