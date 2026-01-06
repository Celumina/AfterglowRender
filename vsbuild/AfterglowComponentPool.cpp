#include "AfterglowComponentPool.h"

AfterglowComponentPool::AfterglowComponentPool() {
	initializeComponentTypes<reg::RegisteredComponentTypes>();
}
