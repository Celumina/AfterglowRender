#include "TerrainCommon.hlsl"
#include "../MeshGeneration.hlsl"

[numthreads(256, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	/* About indexPack
		One indexPack contains 4 indices, in a ideal case, 
		numIndexPacks == 6279174 / 4 == 1569793.5;
		But we can't apply a non-integer numElements, so we should ceil it.
		ceil(1569793.5) == 1569794;
		But 1569794 * [PackIndexCount]4 / [TriangleTopology]3 is not a integer, 
		So ceil it to 1569795.
	*/
	IndexBufferOut[threadID.x].indexPack = GridIndexPack(threadID.x, terrainMeshSideElements);
}