#pragma once
#include <unordered_map>
#include <unordered_set>

#include "AfterglowCommandPool.h"
#include "AfterglowGraphicsQueue.h"
#include "AfterglowReferenceCounter.h"
#include "AfterglowSynchronizer.h"
#include "DebugUtilities.h"

struct AfterglowSharedPoolResource {
	AfterglowReferenceCount count;
};

template<typename KeyType, typename ResourceType>
class AfterglowResourceReference {
public:
	using Key = KeyType;
	using Resource = ResourceType;
	using Resources = std::unordered_map<Key, Resource>;
	AfterglowResourceReference(const Key& key, Resources& resources, AfterglowReferenceCount& count);
	AfterglowResourceReference(const AfterglowResourceReference& other);
	AfterglowResourceReference(AfterglowResourceReference&&) noexcept = default;

protected:
	// @deprecated: std::unordered_map rehash will not change the element address.
	//inline void verifyValue();
	//Key _key;
	Resource* _value;

private:
	// @deprecated: std::unordered_map rehash will not change the element address.
	//Resources& _resources;
	//// Store reference for high frequency query.
	//size_t _oldBucketCount;

	AfterglowReferenceCounter _counter;
};


template <typename ResourceReferenceType>
class AfterglowSharedResourcePool {
public:
	using Key = ResourceReferenceType::Key;
	using Resource = ResourceReferenceType::Resource;
	using Resources = ResourceReferenceType::Resources;

	AfterglowSharedResourcePool(
		AfterglowCommandPool& commandPool, 
		AfterglowGraphicsQueue& graphicsQueue, 
		AfterglowSynchronizer& synchronizer
	);

	AfterglowCommandPool& commandPool();
	AfterglowGraphicsQueue& graphicsQueue();

	void update();

protected:
	// TODO: should be append back if one frame changed....or check exist when delete
	inline void removeResource(const Key* key);

	Resources _resources;
	std::unordered_set<const Key*> _removingCache;

private:
	AfterglowCommandPool& _commandPool;
	AfterglowGraphicsQueue& _graphicsQueue;
	AfterglowSynchronizer& _synchronizer;
};


template<typename KeyType, typename ResourceType>
inline AfterglowResourceReference<KeyType, ResourceType>::AfterglowResourceReference(
	const KeyType& key, 
	Resources& resources, 
	AfterglowReferenceCount& count) : 
	// _key(key),
	// _resources(resources),
	// _oldBucketCount(resources.bucket_count()),
	_value(&resources.at(key)),
	_counter(count) {
}

template<typename KeyType, typename ResourceType>
inline AfterglowResourceReference<KeyType, ResourceType>::AfterglowResourceReference(const AfterglowResourceReference& other) :
	// _key(other._key),
	// _resources(other._resources),
	// _oldBucketCount(other._oldBucketCount),
	_value(other._value),
	_counter(other._counter) {
}

// @deprecated: std::unordered_map rehash will not change the element address.
//template<typename KeyType, typename ResourceType>
//inline void AfterglowResourceReference<KeyType, ResourceType>::verifyValue() {
//	if (_oldBucketCount != _resources.bucket_count()) {
//		DEBUG_CLASS_INFO("Resource pool was rehashed.");
//		_value = &_resources.at(_key);
//		_oldBucketCount = _resources.bucket_count();
//	}
//}

template<typename ResourceReferenceType>
inline AfterglowSharedResourcePool<ResourceReferenceType>::AfterglowSharedResourcePool(
	AfterglowCommandPool& commandPool, 
	AfterglowGraphicsQueue& graphicsQueue, 
	AfterglowSynchronizer& synchronizer) :
	_commandPool(commandPool),
	_graphicsQueue(graphicsQueue), 
	_synchronizer(synchronizer) {
	
}

template<typename ResourceReferenceType>
inline AfterglowCommandPool& AfterglowSharedResourcePool<ResourceReferenceType>::commandPool() {
	return _commandPool;
}

template<typename ResourceReferenceType>
inline AfterglowGraphicsQueue& AfterglowSharedResourcePool<ResourceReferenceType>::graphicsQueue() {
	return _graphicsQueue;
}

template<typename ResourceReferenceType>
inline void AfterglowSharedResourcePool<ResourceReferenceType>::update() {
	if (_removingCache.empty()) {
		return;
	}
	// GPU synchronization.
	_synchronizer.wait(AfterglowSynchronizer::FenceFlag::ComputeInFlight);
	_synchronizer.wait(AfterglowSynchronizer::FenceFlag::RenderInFlight);
	
	for (const auto* key : _removingCache) {
		auto iterator = _resources.find(*key);
		if (iterator->second.count.count() <= 0) {
			_resources.erase(iterator);
		}
		else {
			DEBUG_CLASS_INFO("Resource reference swap was happen.");
		}
	}
	_removingCache.clear();
}

template<typename ResourceReferenceType>
inline void AfterglowSharedResourcePool<ResourceReferenceType>::removeResource(const Key* key) {
	_removingCache.insert(key);
}
