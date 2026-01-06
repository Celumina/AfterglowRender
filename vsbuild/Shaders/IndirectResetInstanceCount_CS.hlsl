[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	// @note: Compute shader generated mesh indexCount should be applied manually. 
	IndirectBufferOut[0].indexCount = indexCount;
	IndirectBufferOut[0].instanceCount = 0;
}