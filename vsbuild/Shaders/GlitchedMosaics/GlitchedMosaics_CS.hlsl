[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	static const float uvOffset = 0.0001;
	static const float2 uv = float2(threadID.xy) / 512.0 + float2(0.0, time * 0.1);
	
	static const uint rayMatchingStep = 16.0;
	static const float2 rayMatchingInterval = { 0.012, 0.005 };
	static const float rayIntensity = 0.001;

	float4 mosaicNoiseMask0 = MosaicNoiseMaskIn.SampleLevel(MosaicNoiseMaskInSampler, (uv * float2(1.73, 0.92) + time * float2(0.3, 0.05)), 0);
	float4 mosaicNoiseMask1 = MosaicNoiseMaskIn.SampleLevel(MosaicNoiseMaskInSampler, (uv * float2(2.4, 0.84) + time * float2(-0.25, -0.08)), 0);
	
	// float4 mosaicNoiseMask = lerp(mosaicNoiseMask0, mosaicNoiseMask1, abs(frac(time) - 0.5) * 2.0);
	float4 mosaicNoiseMask = mosaicNoiseMask0 * mosaicNoiseMask1;

	float4 mosaicNoise = MosaicNoiseIn[threadID.xy];
	MosaicMaskedNoiseOut[threadID.xy] = mosaicNoise * round(mosaicNoiseMask);
	
	float4 mosaicNoise0 = MosaicMaskedNoiseIn.SampleLevel(MosaicMaskedNoiseInSampler, uv + float2(uvOffset, 0.0), 0);
	float4 mosaicNoise1 = MosaicMaskedNoiseIn.SampleLevel(MosaicMaskedNoiseInSampler, uv + float2(-uvOffset, 0.0), 0);
	float4 mosaicNoise2 = MosaicMaskedNoiseIn.SampleLevel(MosaicMaskedNoiseInSampler, uv + float2(0.0, uvOffset), 0);
	float4 mosaicNoise3 = MosaicMaskedNoiseIn.SampleLevel(MosaicMaskedNoiseInSampler, uv + float2(0.0, -uvOffset), 0);


	float4 partial = abs(mosaicNoise0 - mosaicNoise1) + abs(mosaicNoise2 - mosaicNoise3);
	// Pow by illuminance.
	partial = partial * pow((partial.x + partial.y + partial.z) * 16.0, 2.0) * 16.0;

	// Ray tails
	float4 rayAccumulation = 0.0;
	[unroll] for (uint i = 0; i < rayMatchingStep; ++i) {
		float4 rayMosaic = MosaicMaskedNoiseIn.SampleLevel(MosaicMaskedNoiseInSampler, uv + rayMatchingInterval * (i + 1), 0);
		if (any(rayMosaic > 0.0) && (all(mosaicNoise0.xyz == 0.0))) {
			rayAccumulation += (rayMatchingStep - i);// * rayMosaic;
		}
	}

	float4 finalColor = partial * 8.0 + mosaicNoise0 * 0.1 + rayAccumulation * rayIntensity; 

	PresentOut[threadID.xy] = finalColor;
}