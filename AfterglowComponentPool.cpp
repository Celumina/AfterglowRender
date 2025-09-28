#include "AfterglowComponentPool.h"

AfterglowComponentPool::AfterglowComponentPool() {
	initializeComponentTypes<reg::RegisteredComponentTypes>();
}

AfterglowComponentPool::UniqueLock AfterglowComponentPool::lock() const {
	return UniqueLock{_mutex};
}