#include "AfterglowQueue.h"

AfterglowQueue::AfterglowQueue(AfterglowDevice& device, uint32_t queueFamilyIndex) {
	vkGetDeviceQueue(device, queueFamilyIndex, 0, &_queue);
}

AfterglowQueue::operator VkQueue& () {
	return _queue;
}
