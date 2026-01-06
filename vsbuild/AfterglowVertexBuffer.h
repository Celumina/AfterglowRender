#pragma once
#include <array>

#include "VertexStructs.h"
#include "AfterglowStagingBuffer.h"
#include "AfterglowCommandPool.h"
#include "AfterglowGraphicsQueue.h"

// Typeless handle.
struct AfterglowVertexBufferHandle {
	VkBuffer& buffer;
	uint32_t& vertexCount;
};

// VertexType Must be instantialize from AfterglowVertex.
template<vert::VertexType Type>
class AfterglowVertexBufferTemplate : public AfterglowBuffer<AfterglowVertexBufferTemplate<Type>> {
public:
	using Vertex = Type;
	using AttributeDescriptionArray = std::vector<VkVertexInputAttributeDescription>;
	using Size = uint32_t;

	struct Info {
		Size count;
	};

	AfterglowVertexBufferTemplate(AfterglowDevice& device);

	Size vertexCount() const;

	// These function require binded data were exists.
	// void initVertices(uint64_t vertexCount);
	void bind(std::weak_ptr<vert::VertexData> data);
	std::weak_ptr<const vert::VertexData> vertexData() const;

	template<typename Attribute>
	void set(Size pos, const vert::Vec4<Attribute>& value);

	template<typename Attribute>
	static constexpr VkFormat attributeFormat();

	// Creating a staging buffer and use it copy memery to GPU, then clear stagingbuffer.
	void submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);

	constexpr static VkVertexInputBindingDescription getBindingDescription();
	static AttributeDescriptionArray getAttributeDescriptions();


	AfterglowVertexBufferHandle handle();

protected:
	uint64_t byteSize() override;

private:
	using Parent = AfterglowBuffer<AfterglowVertexBufferTemplate<Type>>;

	static constexpr void fillAttributeDescriptions(AttributeDescriptionArray& destDescriptionArray);

	template<typename Attribute>
	static constexpr void fillAttributeDescriptionsImpl(AttributeDescriptionArray& destDescriptionArray);

	inline std::shared_ptr<vert::VertexData> lockedData();

	std::weak_ptr<vert::VertexData> _vertexData;
	Info _info;
};


using AfterglowVertexBuffer = AfterglowVertexBufferTemplate<vert::StandardVertex>;


template<vert::VertexType Type>
template<typename Attribute>
inline void AfterglowVertexBufferTemplate<Type>::set(Size pos, const vert::Vec4<Attribute>& value) {
	reinterpret_cast<Vertex*>(&(*lockedData())[sizeof(Vertex) * pos])->set<Attribute>(Parent::value);
}

template<vert::VertexType Type>
template<typename Attribute>
inline constexpr VkFormat AfterglowVertexBufferTemplate<Type>::attributeFormat() {
	// Here we use medium precision format only (8 bit).
	if constexpr (Attribute::numComponents == 1) {
		return VK_FORMAT_R32_SFLOAT;
	}
	else if constexpr (Attribute::numComponents == 2) {
		return VK_FORMAT_R32G32_SFLOAT;
	}
	else if constexpr (Attribute::numComponents == 3) {
		return VK_FORMAT_R32G32B32_SFLOAT;
	}
	else if constexpr (Attribute::numComponents == 4) {
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	DEBUG_TYPE_WARNING(AfterglowVertexBufferTemplate<Type>, "Unsupported attribute component count.");
	return VK_FORMAT_UNDEFINED;
}

template<vert::VertexType Type>
template<typename Attribute>
static constexpr inline void AfterglowVertexBufferTemplate<Type>::fillAttributeDescriptionsImpl(AttributeDescriptionArray& destAttibuteDescriptions) {
	constexpr auto attrIndex = Vertex::template index<Attribute>();
	destAttibuteDescriptions[attrIndex].location = attrIndex;
	// Binding for different vertex buffer, Keep it 0 until multiple binding support.
	destAttibuteDescriptions[attrIndex].binding = 0;
	// According to vertex format like, using Vertex = AfterglowVertex<float, vert::Position, vert::Color>.
	destAttibuteDescriptions[attrIndex].format = attributeFormat<Attribute>();
	destAttibuteDescriptions[attrIndex].offset = Vertex::template byteOffset<Attribute>();

	if constexpr (!std::is_same_v<typename Vertex::template Next<Attribute>, typename Vertex::Empty>) {
		fillAttributeDescriptionsImpl<typename Vertex::template Next<Attribute>>(destAttibuteDescriptions);
	}
}

template<vert::VertexType Type>
inline AfterglowVertexBufferTemplate<Type>::AfterglowVertexBufferTemplate(AfterglowDevice& device) :
	Parent(device), _info({}) {
	Parent::info().usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
}

template<vert::VertexType Type>
inline void AfterglowVertexBufferTemplate<Type>::bind(std::weak_ptr<vert::VertexData> data) {
	if (Parent::_memory) {
		throw Parent::runtimeError("Indices had been created early.");
	}
	_vertexData = data;
	_info.count = static_cast<Size>(lockedData()->size() / sizeof(Vertex));

	Parent::info().size = byteSize();
	Parent::initMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

template<vert::VertexType Type>
inline AfterglowVertexBufferTemplate<Type>::Size AfterglowVertexBufferTemplate<Type>::vertexCount() const {
	return _info.count;
}

template<vert::VertexType Type>
inline std::weak_ptr<const vert::VertexData> AfterglowVertexBufferTemplate<Type>::vertexData() const {
	return _vertexData;
}

template<vert::VertexType Type>
inline uint64_t AfterglowVertexBufferTemplate<Type>::byteSize() {
	return lockedData()->size();
}

template<vert::VertexType Type>
inline void AfterglowVertexBufferTemplate<Type>::submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue) {
	// Specialization
	// stagingBuffer: host_vk_buffer (CPU)
	// _vertexBuffer(this): vertex_source_array (CPU), local_device_vk_buffer (GPU).
	AfterglowStagingBuffer stagingBuffer(Parent::_device, lockedData()->data(), byteSize());
	commandPool.allocateSingleCommand(
		graphicsQueue,
		[this, &stagingBuffer](VkCommandBuffer commandBuffer) {Parent::cmdCopyBuffer(commandBuffer, stagingBuffer); }
	);
	// Vertices is presistantly map to GPU, so NEVER clear then.
}

template<vert::VertexType Type>
inline constexpr VkVertexInputBindingDescription AfterglowVertexBufferTemplate<Type>::getBindingDescription() {
	return VkVertexInputBindingDescription {
		.binding = 0, 
		.stride = sizeof(Vertex), 
		// Move to the next data entry after each vertex.
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};
}

template<vert::VertexType Type>
inline AfterglowVertexBufferTemplate<Type>::AttributeDescriptionArray AfterglowVertexBufferTemplate<Type>::getAttributeDescriptions() {
	AttributeDescriptionArray attributeDescriptions( Type::count(), VkVertexInputAttributeDescription{});
	fillAttributeDescriptions(attributeDescriptions);
	return attributeDescriptions;
}

template<vert::VertexType Type>
inline AfterglowVertexBufferHandle AfterglowVertexBufferTemplate<Type>::handle() {
	if (!Parent::isDataExists()) {
		throw Parent::runtimeError("Vertex data have not been binded.");
	}

	return AfterglowVertexBufferHandle{
		.buffer = Parent::operator VkBuffer&(), 
		.vertexCount = _info.count
	};
}

template<vert::VertexType Type>
constexpr void AfterglowVertexBufferTemplate<Type>::fillAttributeDescriptions(AttributeDescriptionArray& destDescriptionArray) {
	if constexpr (!std::is_same_v<Vertex::First, Vertex::Empty>) {
		fillAttributeDescriptionsImpl<Vertex::First>(destDescriptionArray);
	}
}

template<vert::VertexType Type>
inline std::shared_ptr<vert::VertexData> AfterglowVertexBufferTemplate<Type>::lockedData() {
	if (_vertexData.expired()) {
		throw Parent::runtimeError("Vertices were expired, due to vertex data source were destructed.");
	}
	auto locked = _vertexData.lock();
	if (!locked) {
		throw Parent::runtimeError("Vertices were not found, due to vertex data source were not binded.");
	}
	return locked;
}

