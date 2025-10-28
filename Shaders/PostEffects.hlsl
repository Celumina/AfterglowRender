#pragma once 

// TODO: Seperate fore, mid and background.
// TODO: Just for test, calculate blur from ComputeShader to improve performance.
float3 SceneColorLinearBlur(float2 uv, float depth) {
	static const float near = 0.333;
	static const float far = 0.666;

	float depthWeight = max(0.0, (depth - 0.999) * 1000.0);
	float focuWeight = saturate(Pow5(depthWeight) + Pow5(200.0 * (1.0 - depth)));

	int2 screenPos = uv * screenResolution;
	// Average blur
	float3 avgBlur = 0.0;
	int kernelWidth = int(6.0 * focuWeight) * 2 + 2;
	// static const int kernelWidth = 24;
	static const int loadInterval = 4;
	for (int indexX = -kernelWidth / 2 + 1; indexX < kernelWidth / 2; ++indexX) {
		for (int indexY = -kernelWidth / 2 + 1; indexY < kernelWidth / 2; ++indexY) {
			// Box Filter
			float boxWeight = (1.0 / float(kernelWidth * kernelWidth));
			// Tent Filter
			float2 tent = float2(kernelWidth - abs(int2(indexX, indexY))) / (float(kernelWidth) - 0.5);
			float tentWeight = tent.x * tent.y * boxWeight * 2;
			// float interestingWeight = (((kernelWidth / 4) - abs(indexX))) * (((kernelWidth / 4) - abs(indexY))) * boxWeight;
			avgBlur += 
				sceneColorTexture.Load(uint3(screenPos + loadInterval * int2(indexX, indexY), 0))
				* tentWeight;
		}
	}
	return avgBlur;
}

float3 SceneColorSensorNoise() {
	// TODO
	return 0;
}