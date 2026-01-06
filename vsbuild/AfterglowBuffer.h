#pragma once

#include <string>
#include "AfterglowDeviceMemory.h"
#include "AfterglowPhysicalDevice.h"

namespace buffer {
	template<typename Type>
	concept BufferType = 
		requires { typename Type::Derived; }
		&& std::is_base_of_v<AfterglowProxyObject<typename Type::Derived, VkBuffer, VkBufferCreateInfo>, Type>;
};

template<typename DerivedType>
class AfterglowBuffer : public AfterglowProxyObject<DerivedType, VkBuffer, VkBufferCreateInfo> {
public:
	// Why explicit prefix is required when call parent functions in CRTP  in successive derived class?
	using Parent = AfterglowProxyObject<DerivedType, VkBuffer, VkBufferCreateInfo>;

	AfterglowBuffer(AfterglowDevice& device);
	~AfterglowBuffer();

proxy_base_protected(Parent):
	void initCreateInfo();
	void create();

protected:
	// TODO: make it as CRTP Function?
	// Rewrite this function to return buffer byte size.
	virtual uint64_t byteSize() = 0;

	void initMemory(VkMemoryPropertyFlags properties);

	// Command functions.
	void cmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcStagingBuffer, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

	AfterglowDevice& _device;
	AfterglowDeviceMemory::AsElement _memory;

private:
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};


template <typename DerivedType>
inline AfterglowBuffer<DerivedType>::AfterglowBuffer(AfterglowDevice& device) :
	_device(device) {
}

template <typename DerivedType>
inline AfterglowBuffer<DerivedType>::~AfterglowBuffer() {
	// If BufferType is not VkBuffer, you should call destroy function manully
	Parent::destroy(vkDestroyBuffer, _device, *this, nullptr);
}

template<typename DerivedType>
inline void AfterglowBuffer<DerivedType>::cmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcStagingBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = srcOffset;
	copyRegion.dstOffset = dstOffset;
	copyRegion.size = byteSize();
	vkCmdCopyBuffer(commandBuffer, srcStagingBuffer, *this, 1, &copyRegion);
}

template <typename DerivedType>
inline void AfterglowBuffer<DerivedType>::initMemory(VkMemoryPropertyFlags properties) {
	VkMemoryRequirements memoryRequirements;
	// Remind that data() could not create automatically, so we use *this.
	vkGetBufferMemoryRequirements(_device, *this, &memoryRequirements);
	_memory = AfterglowDeviceMemory::makeElement(_device);
	_memory->allocationSize = memoryRequirements.size;
	_memory->memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

	// Bind buffer with memory.
	if (vkBindBufferMemory(_device, Parent::data(), _memory, 0) != VK_SUCCESS) {
		throw Parent::runtimeError("Faild to bind buffer memory.");
	}
}

template <typename DerivedType>
inline void AfterglowBuffer<DerivedType>::initCreateInfo() {
	// Fill CreateInfo in VertexBufferManager or DerivedType.
	Parent::info().sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	Parent::info().sharingMode = VK_SHARING_MODE_EXCLUSIVE;
}

template <typename DerivedType>
inline void AfterglowBuffer<DerivedType>::create() {
	// Delay to aquire byte size, to wait the buffer initialization.
	Parent::info().size = byteSize();
	// If BufferType is not VkBuffer, you should implement a custom create function instead.
	if (vkCreateBuffer(_device, &Parent::info(), nullptr, &Parent::data()) != VK_SUCCESS) {
		throw Parent::runtimeError("Failed to create buffer.");
	}
}

template <typename DerivedType>
inline uint32_t AfterglowBuffer<DerivedType>::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(_device.physicalDevice(), &memoryProperties);
	for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
		if ((typeFilter & (1 << index)) && (memoryProperties.memoryTypes[index].propertyFlags & properties)) {
			return index;
		}
	}
	throw Parent::runtimeError("Failed to find suitable memory type.");
}
