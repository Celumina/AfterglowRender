[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	// RGB: Surface Color
	// A: Unused
	TerrainSurfaceOut[threadID.xy] = 0.0;
}
 
