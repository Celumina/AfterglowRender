template<typename Type>
Type zigzagIndex(Type index, Type count) {
	Type maxIndex = count - 1;
	return abs(fmod(index + maxIndex, maxIndex * 2) - maxIndex);
}

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {

	// @note: Generate normal map here due to the initializer is not ensure the memory barrier. 
	// so artifacts were occur in high performance parallel GPUs.
	uint2 texRes = uint2(256, 256);
	float2 adjacentValue = float2(
		FractalNoiseOut[zigzagIndex(threadID.xy + uint2(1, 0), texRes)].x, 
		FractalNoiseOut[zigzagIndex(threadID.xy + uint2(0, 1), texRes)].x
	);
	float2 deltaValue = (FractalNoiseOut[threadID.xy].x - adjacentValue);
	FractalNormalOut[threadID.xy].xy = normalize(float3(deltaValue, 1.0)).xy;
}