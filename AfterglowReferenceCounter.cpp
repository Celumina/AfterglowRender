#include "AfterglowReferenceCounter.h"

AfterglowReferenceCount::AfterglowReferenceCount(
	std::function<IncreaseCallback> increaseCallback,
	std::function<DecreaseCallback> decreaseCallback
	) : _increaseCallback(increaseCallback), _decreaseCallback(decreaseCallback) {
}

void AfterglowReferenceCount::increase() {
	++_count;
	if (_increaseCallback) { 
		_increaseCallback(_count); 
	}
}

void AfterglowReferenceCount::decrease() {
	--_count;
	if (_decreaseCallback) { 
		_decreaseCallback(_count); 
	}
}

AfterglowReferenceCount::Count AfterglowReferenceCount::count() const {
	return _count;
}

void AfterglowReferenceCount::setIncreaseCallback(std::function<IncreaseCallback> increaseCallback) {
	_increaseCallback = increaseCallback;
}

void AfterglowReferenceCount::setDecreaseCallback(std::function<DecreaseCallback> decreaseCallback) {
	_decreaseCallback = decreaseCallback;
}

AfterglowReferenceCounter::AfterglowReferenceCounter(AfterglowReferenceCount& count) : 
	_count(count) {
	_count.increase();
}

AfterglowReferenceCounter::AfterglowReferenceCounter(const AfterglowReferenceCounter& other) : 
	_count(other._count) {
	_count.increase();
}

AfterglowReferenceCounter::~AfterglowReferenceCounter() {
	_count.decrease();
}

AfterglowReferenceCount& AfterglowReferenceCounter::count() {
	return _count;
}

const AfterglowReferenceCount& AfterglowReferenceCounter::count() const {
	return _count;
}