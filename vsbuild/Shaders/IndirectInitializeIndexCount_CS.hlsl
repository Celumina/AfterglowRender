[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	// TODO: The indexCount had not been initialize when the first time initializer invocation.
	// IndirectBufferOut[0].indexCount = indexCount;
	// if (indexCount== 0) {IndirectBufferOut[0].indexCount=600;}
}
	
