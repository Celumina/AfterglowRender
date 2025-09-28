#pragma once
#include <unordered_map>

#include "AfterglowCommandPool.h"
#include "AfterglowGraphicsQueue.h"
#include "AfterglowReferenceCounter.h"
#include "DebugUtilities.h"

template<typename KeyType, typename ResourceType>
class AfterglowResourceReference {
public:
	using Key = KeyType;
	using Resource = ResourceType;
	using Resources = std::unordered_map<Key, Resource>;
	AfterglowResourceReference(const Key& key, Resources& resources, AfterglowReferenceCount& count);
	AfterglowResourceReference(const AfterglowResourceReference& other);

protected:
	inline void verifyValue();
	Key _key;
	Resource* _value;

private:
	Resources& _resources;

	// Store reference for high frequency query.
	size_t _oldBucketCount;

	AfterglowReferenceCounter _counter;
};


template  <typename ResourceReferenceType>
class AfterglowSharedResourcePool {
public:
	using Key = ResourceReferenceType::Key;
	using Resource = ResourceReferenceType::Resource;
	using Resources = ResourceReferenceType::Resources;

	AfterglowSharedResourcePool(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue);

	AfterglowCommandPool& commandPool();
	AfterglowGraphicsQueue& graphicsQueue();

protected:
	Resources _resources;

private:
	AfterglowCommandPool& _commandPool;
	AfterglowGraphicsQueue& _graphicsQueue;
};


template<typename KeyType, typename ResourceType>
inline AfterglowResourceReference<KeyType, ResourceType>::AfterglowResourceReference(
	const KeyType& key, 
	Resources& resources, 
	AfterglowReferenceCount& count) : 
	_key(key),
	_resources(resources),
	_oldBucketCount(resources.bucket_count()),
	_value(&resources.at(key)),
	_counter(count) {
}

template<typename KeyType, typename ResourceType>
inline AfterglowResourceReference<KeyType, ResourceType>::AfterglowResourceReference(const AfterglowResourceReference& other) :
	_key(other._key),
	_resources(other._resources),
	_oldBucketCount(other._oldBucketCount),
	_value(other._value),
	_counter(other._counter) {
}

template<typename KeyType, typename ResourceType>
inline void AfterglowResourceReference<KeyType, ResourceType>::verifyValue() {
	if (_oldBucketCount != _resources.bucket_count()) {
		DEBUG_CLASS_INFO("Resource pool was rehashed.");
		_value = &_resources.at(_key);
		_oldBucketCount = _resources.bucket_count();
	}
}

template<typename ResourceReferenceType>
inline AfterglowSharedResourcePool<ResourceReferenceType>::AfterglowSharedResourcePool(
	AfterglowCommandPool& commandPool, 
	AfterglowGraphicsQueue& graphicsQueue) : 
	_commandPool(commandPool),
	_graphicsQueue(graphicsQueue) {
	
}

template<typename ResourceReferenceType>
inline AfterglowCommandPool& AfterglowSharedResourcePool<ResourceReferenceType>::commandPool() {
	return _commandPool;
}

template<typename ResourceReferenceType>
inline AfterglowGraphicsQueue& AfterglowSharedResourcePool<ResourceReferenceType>::graphicsQueue() {
	return _graphicsQueue;
}
