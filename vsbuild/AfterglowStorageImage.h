#pragma once
#include "AfterglowImage.h"
#include "ComputeDefinitions.h"

class AfterglowCommandPool;
class AfterglowGraphicsQueue;
class AfterglowStagingBuffer;

class AfterglowStorageImage : public AfterglowImage<AfterglowStorageImage> {
public:
	AfterglowStorageImage(
		AfterglowDevice& device, 
		VkExtent3D extent, 
		VkFormat format, 
		compute::SSBOTextureDimension dimension = compute::SSBOTextureDimension::Texture2D, 
		compute::SSBOTextureSampleMode sampleMode = compute::SSBOTextureSampleMode::LinearRepeat
	);

	void submit(AfterglowCommandPool& commandPool, AfterglowGraphicsQueue& graphicsQueue, AfterglowStagingBuffer& stagingBuffer);

};

