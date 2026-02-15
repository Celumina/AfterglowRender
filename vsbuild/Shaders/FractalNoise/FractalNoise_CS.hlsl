template<typename Type>
Type zigzagIndex(Type index, Type count) {
	Type maxIndex = count - 1;
	return abs(fmod(index + maxIndex, maxIndex * 2) - maxIndex);
}

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	// .rg: Simplex noise normal
	// .ba: Worley noise normal
	// @note: Generate normal map here due to the initializer is not ensure the memory barrier. 
	// so artifacts were occur in high performance parallel GPUs.
	uint2 texRes = uint2(256, 256);

	float4 adjacentValue = float4(
		FractalSimplexNoiseOut[zigzagIndex(threadID.xy + uint2(1, 0), texRes)].x, 
		FractalSimplexNoiseOut[zigzagIndex(threadID.xy + uint2(0, 1), texRes)].x, 
		FractalWorleyNoiseOut[zigzagIndex(threadID.xy + uint2(1, 0), texRes)].x, 
		FractalWorleyNoiseOut[zigzagIndex(threadID.xy + uint2(0, 1), texRes)].x
	);
	float4 deltaValue = (float4(FractalSimplexNoiseOut[threadID.xy].xx, FractalWorleyNoiseOut[threadID.xy].xx) - adjacentValue);
	FractalNormalOut[threadID.xy] = float4(normalize(float3(deltaValue.xy, 1.0)).xy, normalize(float3(deltaValue.zw, 1.0)).xy);
}